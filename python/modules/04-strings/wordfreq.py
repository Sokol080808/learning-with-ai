# ЭТАЛОННОЕ РЕШЕНИЕ (ветка reference). На ветке main здесь лежит стаб с NotImplementedError —
# его заполняет ученик. Этот файл существует только чтобы доказать, что задачи решаемы и что
# тесты (включая рандомизированные) зелёные на правильном коде. В main он НЕ попадает.


def word_count(text: str) -> dict[str, int]:
    """Частоты слов в тексте.

    Слова разделяются пробелами (split()), регистр не учитывается (lower()).
    "a A a b" -> {"a": 3, "b": 1}. Пустой текст -> {}.
    """
    counts: dict[str, int] = {}
    for word in text.lower().split():
        counts[word] = counts.get(word, 0) + 1
    return counts


def top_n(text: str, n: int) -> list[tuple[str, int]]:
    """n самых частых слов как пары (слово, частота).

    Порядок: по убыванию частоты, при равной частоте — по алфавиту (возрастание).
    Если уникальных слов меньше n — вернуть сколько есть.
    """
    counts = word_count(text)
    ordered = sorted(counts.items(), key=lambda kv: (-kv[1], kv[0]))
    return ordered[:n]
