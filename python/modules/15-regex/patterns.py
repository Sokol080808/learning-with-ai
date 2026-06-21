# ANSWER KEY — ветка reference.
# На ветке main этот файл содержит стабы с raise NotImplementedError.
# Не публикуй этот файл учащемуся напрямую.

import re


def find_all_numbers(text: str) -> list[int]:
    """Найти все целые числа (с возможным минусом) в тексте.

    Parameters
    ----------
    text : str
        Произвольный текст.

    Returns
    -------
    list[int]
        Числа в порядке появления. Пустой список, если чисел нет.

    Examples
    --------
    >>> find_all_numbers("цена: -10, скидка: 3, остаток: 500")
    [-10, 3, 500]
    >>> find_all_numbers("нет цифр здесь")
    []
    """
    return [int(s) for s in re.findall(r"-?\d+", text)]


def is_valid_identifier(name: str) -> bool:
    """Проверить, является ли строка допустимым Python-идентификатором.

    Правило: начинается с [a-zA-Z_], затем нуль или более [a-zA-Z0-9_].
    Реализовано через re.fullmatch (не через str.isidentifier, чтобы
    исключить юникодные буквы).

    Parameters
    ----------
    name : str
        Проверяемая строка.

    Returns
    -------
    bool

    Examples
    --------
    >>> is_valid_identifier("my_var")
    True
    >>> is_valid_identifier("2bad")
    False
    >>> is_valid_identifier("")
    False
    """
    return bool(re.fullmatch(r"[a-zA-Z_][a-zA-Z0-9_]*", name))


def slugify(title: str) -> str:
    """Преобразовать заголовок статьи в URL-slug.

    Шаги:
    1. Нижний регистр.
    2. Любая последовательность символов, не являющихся [a-z0-9], → дефис.
    3. Убрать ведущий и конечный дефис.

    Parameters
    ----------
    title : str
        Заголовок.

    Returns
    -------
    str
        URL-безопасный slug.

    Examples
    --------
    >>> slugify("  Hello, World!  ")
    'hello-world'
    >>> slugify("Python 3.12 Release")
    'python-3-12-release'
    >>> slugify("---")
    ''
    """
    lowered = title.lower()
    slugged = re.sub(r"[^a-z0-9]+", "-", lowered)
    return slugged.strip("-")


# ---------------------------------------------------------------------------
# Основная задача: парсер строки серверного лога
# ---------------------------------------------------------------------------

# Формат: "YYYY-MM-DD HH:MM:SS LEVEL   [module]   message"
_LOG_PATTERN = re.compile(
    r"(?P<date>\d{4}-\d{2}-\d{2})"
    r"\s+"
    r"(?P<time>\d{2}:\d{2}:\d{2})"
    r"\s+"
    r"(?P<level>\w+)"
    r"\s+"
    r"\[(?P<module>\w+)\]"
    r"\s+"
    r"(?P<message>.+)",
)


def parse_log_line(line: str) -> dict | None:
    """Разобрать одну строку серверного лога.

    Формат строки::

        2026-06-22 14:05:33 ERROR   [auth] Login failed for user alice@example.com

    Parameters
    ----------
    line : str
        Одна строка лога.

    Returns
    -------
    dict | None
        Словарь с ключами ``date``, ``time``, ``level``, ``module``, ``message``.
        Возвращает ``None``, если строка не соответствует формату.

    Examples
    --------
    >>> parse_log_line("2026-06-22 14:05:33 ERROR   [auth] Login failed")
    {'date': '2026-06-22', 'time': '14:05:33', 'level': 'ERROR', 'module': 'auth', 'message': 'Login failed'}
    >>> parse_log_line("не лог") is None
    True
    """
    m = _LOG_PATTERN.fullmatch(line.strip())
    if m is None:
        return None
    return m.groupdict()
