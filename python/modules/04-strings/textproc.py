# ЭТАЛОННОЕ РЕШЕНИЕ (ветка reference). На ветке main здесь лежит стаб с NotImplementedError —
# его заполняет ученик. Этот файл существует только чтобы доказать, что задачи решаемы и что
# тесты (включая рандомизированные) зелёные на правильном коде. В main он НЕ попадает.


def normalize_spaces(s: str) -> str:
    """Схлопнуть любые группы пробелов в один и обрезать края.

    "  привет   мир  " -> "привет мир".
    Пустая строка или строка из одних пробелов -> "".
    """
    return " ".join(s.split())


def is_palindrome(s: str) -> bool:
    """Является ли строка палиндромом без учёта регистра и пробелов.

    "А роза упала на лапу Азора" -> True. Пустая строка -> True.
    """
    cleaned = s.replace(" ", "").lower()
    return cleaned == cleaned[::-1]


def title_case(s: str) -> str:
    """Каждое слово с заглавной буквы, остальные буквы строчные.

    "привет МИР" -> "Привет Мир". Слова разделяются пробелами.
    """
    return " ".join(word.capitalize() for word in s.split(" "))


def count_char(s: str, ch: str) -> int:
    """Сколько раз символ ch встречается в строке s (с учётом регистра)."""
    return s.count(ch)


def safe_decode(data: bytes, encodings: list[str]) -> str:
    """Декодировать байты, перебирая кодировки по очереди; вернуть первую успешную.

    Если ни одна кодировка из списка не подошла (или список пуст) — ValueError.

    Примеры:
        safe_decode(b'hello', ['ascii', 'utf-8'])  -> 'hello'
        safe_decode(b'\\xca\\xee\\xf2', ['utf-8', 'cp1251'])  -> 'Кот'
        safe_decode(b'\\xff', ['utf-8', 'ascii'])  -> ValueError
        safe_decode(b'hello', [])                   -> ValueError
    """
    for enc in encodings:
        try:
            return data.decode(enc)
        except (UnicodeDecodeError, LookupError):
            continue
    raise ValueError(f"Cannot decode bytes with any of the given encodings: {encodings}")
