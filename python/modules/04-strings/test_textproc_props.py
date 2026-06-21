# Эти тесты трогать не нужно — это тоже эталон поведения, но другого рода.
#
# test_textproc.py проверяет ФИКСИРОВАННЫЕ примеры. Здесь — РАНДОМИЗИРОВАННЫЕ
# property-тесты: hypothesis сам подбирает сотни входов (включая злые — пустые строки,
# одни пробелы, юникод, смешанный регистр, повторяющиеся пробелы) и проверяет не
# конкретный ответ, а ИНВАРИАНТЫ, обязательные для ЛЮБОГО входа. Если реализация верна
# на примерах, но ломается на краевом случае, эти тесты покажут минимальный контрпример.

from hypothesis import given, settings
from hypothesis import strategies as st

from textproc import normalize_spaces, is_palindrome, title_case, count_char, safe_decode

text = st.text(max_size=100)


# --- normalize_spaces: ни ведущих/хвостовых пробелов, ни двойных внутри ---

@given(s=text)
def test_normalize_spaces_no_leading_or_trailing_space(s):
    out = normalize_spaces(s)
    assert out == out.strip()


@given(s=text)
def test_normalize_spaces_no_double_spaces(s):
    out = normalize_spaces(s)
    assert "  " not in out


@given(s=text)
def test_normalize_spaces_equals_split_join(s):
    # Сильная проверка корректности: ровно " ".join(s.split()).
    assert normalize_spaces(s) == " ".join(s.split())


@given(s=text)
def test_normalize_spaces_idempotent(s):
    once = normalize_spaces(s)
    assert normalize_spaces(once) == once


@given(s=text)
def test_normalize_spaces_does_not_mutate_input(s):
    original = s
    normalize_spaces(s)
    assert s == original


def test_normalize_spaces_extreme_cases():
    assert normalize_spaces("") == ""
    assert normalize_spaces("     ") == ""
    assert normalize_spaces("  привет   мир  ") == "привет мир"


# --- is_palindrome: разворот без учёта пробелов и регистра ---

@given(s=text)
def test_is_palindrome_returns_real_bool(s):
    assert is_palindrome(s) is True or is_palindrome(s) is False


@given(s=text)
def test_is_palindrome_matches_definition(s):
    # Сильная проверка: убрать пробелы, привести к нижнему — и сравнить с разворотом.
    cleaned = s.replace(" ", "").lower()
    assert is_palindrome(s) is (cleaned == cleaned[::-1])


@given(s=st.text(alphabet="абвгде яё", max_size=40))
def test_is_palindrome_doubled_is_palindrome(s):
    # Строка, склеенная со своим разворотом, всегда палиндром (с точностью до регистра/пробелов).
    base = s.replace(" ", "").lower()
    assert is_palindrome(base + base[::-1]) is True


@given(s=text)
def test_is_palindrome_ignores_case(s):
    assert is_palindrome(s) == is_palindrome(s.upper())


def test_is_palindrome_extreme_cases():
    assert is_palindrome("") is True
    assert is_palindrome("   ") is True
    assert is_palindrome("А роза упала на лапу Азора") is True
    assert is_palindrome("привет") is False


# --- title_case: каждое слово с заглавной, остальные строчные ---

@given(s=text)
def test_title_case_each_word_capitalized(s):
    out = title_case(s)
    for word in out.split(" "):
        if word:
            assert word == word[0].upper() + word[1:].lower()


@given(s=text)
def test_title_case_equals_word_capitalize_join(s):
    # Сильная проверка: ровно " ".join(w.capitalize() for w in s.split(" ")).
    assert title_case(s) == " ".join(w.capitalize() for w in s.split(" "))


@given(s=text)
def test_title_case_idempotent(s):
    once = title_case(s)
    assert title_case(once) == once


def test_title_case_extreme_cases():
    assert title_case("") == ""
    assert title_case("привет МИР") == "Привет Мир"
    assert title_case("Hello World") == "Hello World"


# --- count_char: подсчёт вхождений, регистрозависимо ---

@given(s=text, ch=st.characters())
def test_count_char_matches_str_count(s, ch):
    assert count_char(s, ch) == s.count(ch)


@given(s=text, ch=st.characters())
def test_count_char_is_nonnegative(s, ch):
    assert count_char(s, ch) >= 0


@given(ch=st.characters(), k=st.integers(min_value=0, max_value=50))
def test_count_char_repeated_string(ch, k):
    # Строка из k одинаковых символов содержит ровно k таких символов.
    assert count_char(ch * k, ch) == k


def test_count_char_extreme_cases():
    assert count_char("", "a") == 0
    assert count_char("банан", "а") == 2
    assert count_char("Алла", "а") == 1  # регистрозависимо


# --- safe_decode: round-trip и инварианты ----------------------------------

# Стратегия: генерируем текстовые строки, кодируем в UTF-8, декодируем через safe_decode.
# Инвариант: encode→decode возвращает исходную строку (round-trip).

@given(s=st.text(max_size=80))
@settings(derandomize=True)
def test_safe_decode_roundtrip_utf8(s):
    """encode("utf-8") → safe_decode([..., "utf-8"]) должен вернуть исходную строку."""
    encoded = s.encode("utf-8")
    # Список намеренно начинаем с заведомо узкой кодировки (ascii),
    # чтобы safe_decode мог упасть на первой и перейти к utf-8.
    result = safe_decode(encoded, ["ascii", "utf-8"])
    assert result == s


@given(s=st.text(alphabet=st.characters(whitelist_categories=("L", "N", "P")), max_size=60))
@settings(derandomize=True)
def test_safe_decode_result_is_str(s):
    """safe_decode всегда возвращает str, а не bytes."""
    encoded = s.encode("utf-8")
    result = safe_decode(encoded, ["utf-8"])
    assert isinstance(result, str)


@given(data=st.binary(max_size=60))
@settings(derandomize=True)
def test_safe_decode_latin1_never_raises(data):
    """latin-1 декодирует любую последовательность байтов — исключения нет."""
    result = safe_decode(data, ["utf-8", "latin-1"])
    assert isinstance(result, str)


@given(s=st.text(max_size=80))
@settings(derandomize=True)
def test_safe_decode_empty_encodings_always_raises(s):
    """Пустой список кодировок всегда даёт ValueError."""
    import pytest
    with pytest.raises(ValueError):
        safe_decode(s.encode("utf-8"), [])


def test_safe_decode_extreme_cases():
    # CP1251 «Кот» — первый UTF-8 промахивается, второй cp1251 попадает
    assert safe_decode(b"\xca\xee\xf2", ["utf-8", "cp1251"]) == "Кот"
    # Пустые байты — пустая строка
    assert safe_decode(b"", ["utf-8"]) == ""
    # latin-1 как спасательная сеть
    assert isinstance(safe_decode(b"\xff", ["utf-8", "latin-1"]), str)
