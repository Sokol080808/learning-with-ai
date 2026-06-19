# ВНИМАНИЕ: этот файл — "лицо" пакета mathx. Трогать его обычно не нужно.
#
# __init__.py выполняется ОДИН раз, когда кто-то делает `import mathx`.
# Его задача здесь — собрать публичные функции из подмодулей в одно место,
# чтобы снаружи можно было писать коротко:
#
#     from mathx import add, mul, factorial, fib
#
# вместо длинного `from mathx.arithmetic import add`. Это и есть РЕЭКСПОРТ:
# имена «поднимаются» из подмодулей на уровень пакета.

from .arithmetic import add, mul
from .sequences import factorial, fib
from .statistics import mean, median, mode, variance

# __all__ — список того, что считается публичным API пакета.
# Он же управляет тем, что попадёт при `from mathx import *`.
__all__ = ["add", "mul", "factorial", "fib", "mean", "median", "mode", "variance"]
