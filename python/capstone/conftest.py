# Чтобы тесты в tests/ могли делать `from tasks.store import TaskStore`, каталог
# capstone (где лежит пакет tasks/) должен быть в sys.path. pytest сам кладёт в
# sys.path каталог С КАЖДЫМ conftest.py — поэтому достаточно положить этот файл
# здесь, но добавим путь явно, чтобы запуск работал и вне pytest.

import os
import sys

sys.path.insert(0, os.path.dirname(__file__))
