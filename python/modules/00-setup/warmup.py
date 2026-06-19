# ВНИМАНИЕ: здесь пишешь ТЫ. Реализуй функции так, чтобы тесты модуля 00 стали зелёными.
#
# Сейчас каждая функция кидает NotImplementedError — поэтому тесты падают (red).
# Замени тело на настоящую реализацию, запусти ./python/run.sh 00 и наблюдай green.


def add(a: int, b: int) -> int:
    """Вернуть сумму двух целых чисел.

    Контракт:
      add(2, 3) == 5
      add(-1, 1) == 0
      add(0, 0) == 0
    """
    raise NotImplementedError("TODO: верни сумму a и b")


def seconds_in(hours: int) -> int:
    """Сколько секунд содержится в `hours` часах.

    В одном часе 3600 секунд (60 * 60).
    Контракт:
      seconds_in(0) == 0
      seconds_in(1) == 3600
      seconds_in(2) == 7200
    """
    raise NotImplementedError("TODO: верни число секунд в hours часах")
