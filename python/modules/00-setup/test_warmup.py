# Эти тесты трогать не нужно — это эталон поведения.
# Импорт берёт функции из стаба warmup.py, лежащего рядом (pytest добавляет
# каталог теста в sys.path). Пока стаб не реализован — тесты падают, и это правильно.

from warmup import add, seconds_in


# --- add ---------------------------------------------------------------------

def test_add_positive():
    assert add(2, 3) == 5


def test_add_with_zero():
    assert add(0, 0) == 0
    assert add(7, 0) == 7
    assert add(0, 7) == 7


def test_add_negative():
    assert add(-1, 1) == 0
    assert add(-4, -6) == -10


def test_add_commutative():
    # a + b == b + a — на всякий случай проверяем симметрию
    assert add(8, 5) == add(5, 8)


def test_add_large():
    assert add(1_000_000, 2_000_000) == 3_000_000


# --- seconds_in --------------------------------------------------------------

def test_seconds_in_zero():
    assert seconds_in(0) == 0


def test_seconds_in_one_hour():
    assert seconds_in(1) == 3600


def test_seconds_in_two_hours():
    assert seconds_in(2) == 7200


def test_seconds_in_full_day():
    assert seconds_in(24) == 86_400


def test_seconds_in_is_multiple_of_3600():
    # Что бы ни вернула функция для 5 часов — это ровно 5 * 3600
    assert seconds_in(5) == 5 * 3600
