# Эти тесты трогать не нужно — это эталон поведения.
# Запуск: ./python/run.sh 04

from textproc import normalize_spaces, is_palindrome, title_case, count_char, safe_decode


# --- normalize_spaces -------------------------------------------------------

def test_normalize_spaces_basic():
    assert normalize_spaces("  привет   мир  ") == "привет мир"


def test_normalize_spaces_single_word():
    assert normalize_spaces("слово") == "слово"


def test_normalize_spaces_inner_multiple():
    assert normalize_spaces("a     b      c") == "a b c"


def test_normalize_spaces_trims_edges():
    assert normalize_spaces("   край ") == "край"


def test_normalize_spaces_empty():
    assert normalize_spaces("") == ""


def test_normalize_spaces_only_spaces():
    assert normalize_spaces("     ") == ""


def test_normalize_spaces_returns_new_string():
    src = "  a  b  "
    out = normalize_spaces(src)
    # исходная строка не должна меняться (строки неизменяемы)
    assert src == "  a  b  "
    assert out == "a b"


# --- is_palindrome ----------------------------------------------------------

def test_is_palindrome_simple_true():
    assert is_palindrome("шалаш") is True


def test_is_palindrome_simple_false():
    assert is_palindrome("привет") is False


def test_is_palindrome_ignores_case():
    assert is_palindrome("Шалаш") is True


def test_is_palindrome_ignores_spaces_and_case():
    assert is_palindrome("А роза упала на лапу Азора") is True


def test_is_palindrome_empty_is_true():
    assert is_palindrome("") is True


def test_is_palindrome_single_char():
    assert is_palindrome("я") is True


def test_is_palindrome_only_spaces_is_true():
    # после удаления пробелов остаётся "", а это палиндром
    assert is_palindrome("   ") is True


# --- title_case -------------------------------------------------------------

def test_title_case_basic():
    assert title_case("привет мир") == "Привет Мир"


def test_title_case_lowers_rest():
    assert title_case("привет МИР") == "Привет Мир"


def test_title_case_single_word():
    assert title_case("python") == "Python"


def test_title_case_already_titled():
    assert title_case("Hello World") == "Hello World"


def test_title_case_empty():
    assert title_case("") == ""


# --- count_char -------------------------------------------------------------

def test_count_char_basic():
    assert count_char("банан", "а") == 2


def test_count_char_not_present():
    assert count_char("банан", "z") == 0


def test_count_char_case_sensitive():
    assert count_char("Алла", "а") == 1


def test_count_char_empty_string():
    assert count_char("", "a") == 0


def test_count_char_every_char():
    assert count_char("ааа", "а") == 3


# --- safe_decode ------------------------------------------------------------

def test_safe_decode_ascii_first():
    # ASCII успевает первым — возвращает строку
    assert safe_decode(b"hello", ["ascii", "utf-8"]) == "hello"


def test_safe_decode_falls_back_to_second():
    # Байты КОТ в CP1251; UTF-8 не подходит, CP1251 — подходит
    assert safe_decode(b"\xca\xee\xf2", ["utf-8", "cp1251"]) == "Кот"


def test_safe_decode_utf8_cyrillic():
    # UTF-8 кириллица
    assert safe_decode("Привет".encode("utf-8"), ["utf-8"]) == "Привет"


def test_safe_decode_empty_bytes_utf8():
    # Пустые байты декодируются в пустую строку
    assert safe_decode(b"", ["utf-8"]) == ""


def test_safe_decode_empty_encodings_raises():
    import pytest
    with pytest.raises(ValueError):
        safe_decode(b"hello", [])


def test_safe_decode_all_fail_raises():
    import pytest
    # 0xFF — невалиден и для UTF-8, и для ASCII
    with pytest.raises(ValueError):
        safe_decode(b"\xff", ["utf-8", "ascii"])


def test_safe_decode_bogus_encoding_raises():
    import pytest
    # Несуществующая кодировка — LookupError, должна быть поймана, итог ValueError
    with pytest.raises(ValueError):
        safe_decode(b"hello", ["not-a-real-encoding"])


def test_safe_decode_latin1_always_succeeds():
    # latin-1 (iso-8859-1) декодирует любой байт 0x00..0xFF без ошибок
    result = safe_decode(b"\xff\xfe\xfd", ["utf-8", "latin-1"])
    assert isinstance(result, str)
    assert len(result) == 3


def test_safe_decode_emoji_utf8():
    # Эмодзи корректно передаются через UTF-8 round-trip
    original = "😀🎉"
    encoded = original.encode("utf-8")
    assert safe_decode(encoded, ["ascii", "utf-8"]) == original
