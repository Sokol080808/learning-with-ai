#!/usr/bin/env python3
"""Doc sanity checks for the courses repo — stdlib only, no deps.

Catches the doc-staleness that creeps in when modules are added/renamed:
  1. broken relative markdown links (renamed/moved files);
  2. dangling module-number references — `модуль NN` (or a range) pointing
     past a track's real module range (typos, removed/renumbered modules);
  3. wrong total module count in a track README (best-effort, only when the
     canonical "разбит на **N модулей**" phrasing is present).

It does NOT catch *semantic* mistakes (a reference to a real module that is
the wrong one for the topic) — those still need human review.

Run: `python tools/check_docs.py`  (exit 1 on any issue)
Self-test: `python tools/check_docs.py --selftest`
"""
import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
TRACKS = ["cpp", "python", "rust", "deep-learning", "caos"]
SKIP_DIRS = {".venv", "venv", "target", "build", "node_modules", ".git",
             "graphify-out", "learning-logs", "__pycache__", ".pytest_cache"}

LINK_RE = re.compile(r"\[[^\]]*\]\(([^)]+)\)")
# "модуль 21", "модуля 14", "модулях 15–22", "модули 00–10", "(модуль 12)"
MOD_REF_RE = re.compile(r"модул\w*\s+(\d{1,2})(?:\s*[–—-]\s*(\d{1,2}))?")
# canonical course-size sentence: "...разбит на **18 модулей**"
COUNT_RE = re.compile(r"на\s+\*\*(\d+)\s+модул")


def strip_code(text: str) -> str:
    """Remove fenced ``` blocks and inline `code` so code (e.g. C++ lambdas
    `[](int x)`, subscripts) is not mistaken for markdown links / refs."""
    text = re.sub(r"```.*?```", "", text, flags=re.DOTALL)
    text = re.sub(r"<(code|pre)\b.*?</\1>", "", text, flags=re.DOTALL)
    text = re.sub(r"`[^`]*`", "", text)
    return text


def md_files(base: Path):
    for p in base.rglob("*.md"):
        if any(part in SKIP_DIRS for part in p.parts):
            continue
        yield p


def track_modules(track: str):
    d = ROOT / track / "modules"
    nums = []
    if d.is_dir():
        for sub in d.iterdir():
            m = re.match(r"(\d+)-", sub.name)
            if sub.is_dir() and m:
                nums.append(int(m.group(1)))
    return sorted(nums)


def check_links(p: Path, errors):
    for m in LINK_RE.finditer(strip_code(p.read_text(encoding="utf-8"))):
        target = m.group(1).strip()
        if target.startswith(("http://", "https://", "#", "mailto:")):
            continue
        path_part = target.split("#", 1)[0].split("?", 1)[0].strip()
        if not path_part:
            continue
        if not (p.parent / path_part).resolve().exists():
            errors.append(f"{p.relative_to(ROOT)}: broken link -> {target}")


def check_module_refs(p: Path, track: str, maxmod: int, errors):
    text = strip_code(p.read_text(encoding="utf-8"))
    for m in MOD_REF_RE.finditer(text):
        for g in m.groups():
            if g is None:
                continue
            n = int(g)
            if n > maxmod:
                ctx = " ".join(text[max(0, m.start() - 18):m.end() + 8].split())
                errors.append(
                    f"{p.relative_to(ROOT)}: ref to module {n:02d}, but {track} "
                    f"has only 00–{maxmod:02d}  (…{ctx}…)")


def check_count(track: str, n_modules: int, errors):
    readme = ROOT / track / "README.md"
    if not readme.is_file():
        return
    for m in COUNT_RE.finditer(readme.read_text(encoding="utf-8")):
        if int(m.group(1)) != n_modules:
            errors.append(
                f"{track}/README.md: says '{m.group(1)} модулей' "
                f"but track has {n_modules}")


def selftest():
    assert MOD_REF_RE.findall("см. модуль 21 и модули 15–22") == [("21", ""), ("15", "22")]
    assert MOD_REF_RE.findall("модульный подход") == []
    assert LINK_RE.findall("[a](x/y.md) [b](http://z)") == ["x/y.md", "http://z"]
    assert re.match(r"(\d+)-", "00-setup").group(1) == "00"
    assert COUNT_RE.search("разбит на **18 модулей**").group(1) == "18"
    # code (C++ lambda) must not survive as a link; a real md link must
    assert LINK_RE.findall(strip_code("```\nauto f = [](int x){};\n```")) == []
    assert LINK_RE.findall(strip_code("see [`a.md`](dir/a.md)")) == ["dir/a.md"]
    assert LINK_RE.findall(strip_code("<code>schema[col](row[col])</code>")) == []
    print("selftest OK")


def main():
    if "--selftest" in sys.argv:
        selftest()
        return 0
    errors = []
    for base in [ROOT / t for t in TRACKS]:
        for p in md_files(base):
            check_links(p, errors)
    for name in ("README.md", "todo.md", "CLAUDE.md"):
        p = ROOT / name
        if p.is_file():
            check_links(p, errors)
    for t in TRACKS:
        mods = track_modules(t)
        if not mods:
            continue
        maxmod = max(mods)
        for p in md_files(ROOT / t):
            check_module_refs(p, t, maxmod, errors)
        check_count(t, len(mods), errors)
    if errors:
        print(f"✗ {len(errors)} doc issue(s):")
        for e in errors:
            print("  -", e)
        return 1
    print("✓ docs OK (links, module refs, counts)")
    return 0


if __name__ == "__main__":
    sys.exit(main())
