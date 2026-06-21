# ВНИМАНИЕ: здесь пишешь ТЫ. Реализуй функции так,
# чтобы тесты модуля 04 стали зелёными.
#
# Помни главную идею модуля: строки неизменяемы. Методы строк не «портят» исходную
# строку, а ВОЗВРАЩАЮТ новую — поэтому результат всегда нужно вернуть (return) или
# присвоить. Готовых решений тут нет: сигнатуры и контракт есть, тело — за тобой.


def normalize_spaces(s: str) -> str:
    """Схлопнуть любые группы пробелов в один и обрезать края.

    "  привет   мир  " -> "привет мир".
    Пустая строка или строка из одних пробелов -> "".
    """
    raise NotImplementedError("TODO: реализуй normalize_spaces (см. Идею 1: split/join)")


def is_palindrome(s: str) -> bool:
    """Является ли строка палиндромом без учёта регистра и пробелов.

    "А роза упала на лапу Азора" -> True. Пустая строка -> True.
    """
    raise NotImplementedError("TODO: реализуй is_palindrome (см. Идею 2: разворот [::-1])")


def title_case(s: str) -> str:
    """Каждое слово с заглавной буквы, остальные буквы строчные.

    "привет МИР" -> "Привет Мир". Слова разделяются пробелами.
    """
    raise NotImplementedError("TODO: реализуй title_case (см. Идею 1 и capitalize)")


def count_char(s: str, ch: str) -> int:
    """Сколько раз символ ch встречается в строке s (с учётом регистра)."""
    raise NotImplementedError("TODO: реализуй count_char (см. Идею 4)")


def safe_decode(data: bytes, encodings: list[str]) -> str:
    """Декодировать байты, перебирая кодировки по очереди; вернуть первую успешную.

    Если ни одна кодировка из списка не подошла (или список пуст) — ValueError.

    Примеры:
        safe_decode(b'hello', ['ascii', 'utf-8'])  -> 'hello'
        safe_decode(b'\\xca\\xee\\xf2', ['utf-8', 'cp1251'])  -> 'Кот'
        safe_decode(b'\\xff', ['utf-8', 'ascii'])  -> ValueError
        safe_decode(b'hello', [])                   -> ValueError
    """
    raise NotImplementedError("TODO: реализуй safe_decode — перебери encodings, поймай UnicodeDecodeError/LookupError, если ничего не подошло — подними ValueError")
