# conftest.py — служебный файл pytest. Трогать не нужно.
#
# Он добавляет каталог capstone/ в начало sys.path, чтобы из тестов работал импорт пакета:
#     from db.database import Database, QueryError
#     from db.tokenizer import tokenize
#     from db.parser import parse, Select, Condition, ...
# независимо от того, из какой директории запущен pytest.

import os
import sys

sys.path.insert(0, os.path.dirname(__file__))
