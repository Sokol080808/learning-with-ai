# Этот файл трогать не нужно — он лишь добавляет каталог капстоуна в sys.path,
# чтобы из тестов работал импорт `from nanolm.model import TransformerLM` и т.п.
import os
import sys

sys.path.insert(0, os.path.dirname(__file__))
