# Пакет nanolm — публичный API крошечной языковой модели капстоуна.
#
# Этот файл просто реэкспортирует то, что ты реализуешь в модулях пакета,
# чтобы снаружи можно было писать коротко:
#
#     from nanolm import CharTokenizer, TransformerLM, get_batch
#
# Реализовывать здесь ничего не нужно — вся работа в tokenizer.py, data.py, model.py.

from nanolm.tokenizer import CharTokenizer
from nanolm.data import get_batch
from nanolm.model import TransformerLM

__all__ = ["CharTokenizer", "get_batch", "TransformerLM"]
