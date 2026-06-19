#!/usr/bin/env python3
"""Stop-hook: сохраняет последний обмен «вопрос ученика → ответ преподавателя» в учебный журнал.

Зачем: накопить диалоги во время обучения, чтобы потом улучшать курсы (где ученик застревает,
какие объяснения непонятны, каких подсказок не хватает).

Когда пишет: только на УЧЕБНЫХ ветках — пропускает `main` и `course/*` (там авторство курсов,
а не учёба). Обычно учёба идёт на ветке `solutions`.

Куда пишет (каталог learning-logs/ в корне проекта):
  - journal.jsonl            — канонический машинно-читаемый лог (по строке на обмен)
  - journal-YYYY-MM-DD.md    — человекочитаемый дневник по дням
  - .last_captured           — служебный файл дедупликации (uuid последнего записанного ответа)

Hook получает на stdin JSON со `transcript_path` и `cwd`. Скрипт ОТКАЗОУСТОЙЧИВ: при любой
ошибке тихо выходит с кодом 0 и никогда не ломает сессию.
"""
import sys
import os
import json
import re
import subprocess


def read_payload():
    raw = sys.stdin.read() if not sys.stdin.isatty() else ""
    try:
        return json.loads(raw) if raw.strip() else {}
    except Exception:
        return {}


def load_transcript(path):
    entries = []
    with open(path, "r", encoding="utf-8") as f:
        for line in f:
            line = line.strip()
            if not line:
                continue
            try:
                entries.append(json.loads(line))
            except Exception:
                pass
    return entries


def assistant_text(entry):
    """Текст видимого ответа ассистента: только блоки type=='text' (без thinking и tool_use)."""
    content = (entry.get("message") or {}).get("content")
    if isinstance(content, list):
        parts = [b.get("text", "") for b in content
                 if isinstance(b, dict) and b.get("type") == "text"]
        return "\n".join(p for p in parts if p).strip()
    if isinstance(content, str):
        return content.strip()
    return ""


_NOISE = ("<command-name>", "<local-command-stdout>", "local-command-caveat",
          "<command-message>", "<command-args>", "[Request interrupted")


def human_text(entry):
    """Текст человеческого запроса. Пусто, если это tool_result, команда или системный шум."""
    if entry.get("type") != "user":
        return ""
    content = (entry.get("message") or {}).get("content")
    text = ""
    if isinstance(content, str):
        text = content
    elif isinstance(content, list):
        # человеческий ввод иногда массив с текстовыми блоками; tool_result пропускаем
        if any(isinstance(b, dict) and b.get("type") == "tool_result" for b in content):
            return ""
        text = "\n".join(b.get("text", "") for b in content
                         if isinstance(b, dict) and b.get("type") == "text")
    text = text.strip()
    if not text:
        return ""
    if any(tok in text for tok in _NOISE):
        return ""
    if text.startswith("<") and text.endswith(">"):  # чисто системная обёртка
        return ""
    return text


def latest_exchange(entries):
    """(user_text, assistant_text, assistant_uuid, timestamp) для последнего обмена, или None."""
    last_asst = None
    for i in range(len(entries) - 1, -1, -1):
        if entries[i].get("type") == "assistant" and assistant_text(entries[i]):
            last_asst = i
            break
    if last_asst is None:
        return None
    a = entries[last_asst]
    user = ""
    for i in range(last_asst - 1, -1, -1):
        ht = human_text(entries[i])
        if ht:
            user = ht
            break
    uuid = a.get("uuid") or (a.get("message") or {}).get("id") or ""
    ts = a.get("timestamp") or ""
    return user, assistant_text(a), uuid, ts


def detect_context(text):
    m = re.search(r"(cpp|python|rust|deep-learning|caos)/"
                  r"(?:modules/\d{2}-[a-z0-9-]+|capstone)", text)
    return m.group(0) if m else ""


def current_branch(cwd):
    try:
        return subprocess.run(["git", "-C", cwd, "rev-parse", "--abbrev-ref", "HEAD"],
                              capture_output=True, text=True, timeout=5).stdout.strip()
    except Exception:
        return ""


def capture(cwd, transcript, branch, session=""):
    if not transcript or not os.path.exists(transcript):
        return None
    entries = load_transcript(transcript)
    ex = latest_exchange(entries)
    if not ex:
        return None
    user, asst, uuid, ts = ex
    if not user and not asst:
        return None

    logdir = os.path.join(cwd, "learning-logs")
    os.makedirs(logdir, exist_ok=True)
    statef = os.path.join(logdir, ".last_captured")
    try:
        with open(statef, encoding="utf-8") as f:
            if uuid and f.read().strip() == uuid:
                return None  # этот ответ уже записан
    except Exception:
        pass

    ctx = detect_context(user + "\n" + asst)
    rec = {"ts": ts, "branch": branch, "context": ctx,
           "session": session, "user": user, "assistant": asst}

    with open(os.path.join(logdir, "journal.jsonl"), "a", encoding="utf-8") as f:
        f.write(json.dumps(rec, ensure_ascii=False) + "\n")

    day = ts[:10] if ts else "undated"
    clock = ts[11:16] if len(ts) >= 16 else ""
    md = os.path.join(logdir, f"journal-{day}.md")
    with open(md, "a", encoding="utf-8") as f:
        if os.path.getsize(md) == 0:
            f.write(f"# Учебный дневник — {day}\n\n"
                    f"_Автоматически: вопросы и ответы во время обучения. "
                    f"Используется для улучшения курсов._\n\n")
        head = f"## {clock}" + (f" · `{ctx}`" if ctx else "")
        f.write(head + "\n\n")
        f.write("**Я (ученик):**\n\n" + (user or "_(нет текста)_") + "\n\n")
        f.write("**Преподаватель:**\n\n" + (asst or "_(нет текста)_") + "\n\n---\n\n")

    if uuid:
        with open(statef, "w", encoding="utf-8") as f:
            f.write(uuid)
    return rec


def main():
    # тестовый режим: python capture_dialogue.py --test <transcript.jsonl>
    if len(sys.argv) >= 3 and sys.argv[1] == "--test":
        ex = latest_exchange(load_transcript(sys.argv[2]))
        if not ex:
            print("no exchange found"); return
        u, a, uuid, ts = ex
        print("ts:", ts, "| uuid:", uuid[:12], "| ctx:", detect_context(u + a))
        print("USER:", (u[:200] + "…") if len(u) > 200 else u)
        print("ASSISTANT:", (a[:200] + "…") if len(a) > 200 else a)
        return

    payload = read_payload()
    cwd = payload.get("cwd") or os.getcwd()
    branch = current_branch(cwd)
    if branch == "main" or branch.startswith("course/"):
        return  # авторство курсов — не учёба
    capture(cwd, payload.get("transcript_path"), branch, payload.get("session_id", ""))


if __name__ == "__main__":
    try:
        main()
    except Exception:
        pass
    sys.exit(0)
