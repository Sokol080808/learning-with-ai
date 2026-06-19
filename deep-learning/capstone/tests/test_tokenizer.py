# Эти тесты трогать не нужно — это эталон поведения (приёмочные тесты капстоуна).
#
# Майлстоун 1 — токенизатор и данные. Тесты детерминированы, крошечные, на CPU.

import torch

from nanolm.tokenizer import CharTokenizer
from nanolm.data import get_batch


TEXT = "to be or not to be"


# ---------------------------------------------------------------------------
# CharTokenizer: алфавит, размер словаря, кодирование/декодирование
# ---------------------------------------------------------------------------

def test_vocab_is_sorted_unique_chars():
    tok = CharTokenizer().build(TEXT)
    # Алфавит = отсортированное множество уникальных символов.
    assert tok.vocab_size == len(set(TEXT))


def test_vocab_size_known_value():
    # "to be or not to be" -> уникальные символы: ' ', 'b', 'e', 'n', 'o', 'r', 't'
    tok = CharTokenizer().build(TEXT)
    assert tok.vocab_size == 7


def test_encode_returns_list_of_ints_same_length():
    tok = CharTokenizer().build(TEXT)
    ids = tok.encode(TEXT)
    assert isinstance(ids, list)
    assert len(ids) == len(TEXT)
    assert all(isinstance(i, int) for i in ids)


def test_encode_ids_in_range():
    tok = CharTokenizer().build(TEXT)
    ids = tok.encode(TEXT)
    assert all(0 <= i < tok.vocab_size for i in ids)


def test_roundtrip_decode_encode_identity():
    # Главное свойство токенизатора: decode(encode(text)) == text.
    tok = CharTokenizer().build(TEXT)
    assert tok.decode(tok.encode(TEXT)) == TEXT


def test_roundtrip_on_subtext():
    # round-trip держится и на подстроке (символы из обученного алфавита).
    tok = CharTokenizer().build(TEXT)
    sub = "be not"
    assert tok.decode(tok.encode(sub)) == sub


def test_encode_known_small_example():
    # text = "abca" -> алфавит ['a','b','c'], stoi {'a':0,'b':1,'c':2}
    tok = CharTokenizer().build("abca")
    assert tok.vocab_size == 3
    assert tok.encode("cab") == [2, 0, 1]
    assert tok.decode([2, 0, 1]) == "cab"


def test_same_char_maps_to_same_id():
    # одинаковые символы кодируются одинаковым индексом
    tok = CharTokenizer().build(TEXT)
    ids = tok.encode("tt oo")
    assert ids[0] == ids[1]      # 't' == 't'
    assert ids[3] == ids[4]      # 'o' == 'o'


# ---------------------------------------------------------------------------
# get_batch: формы, типы, инвариант сдвига на 1, детерминизм
# ---------------------------------------------------------------------------

def _make_data(n=100):
    # просто последовательность индексов 0..n-1 — удобно проверять сдвиг
    return torch.arange(n, dtype=torch.long)


def test_get_batch_shapes():
    data = _make_data(100)
    x, y = get_batch(data, block_size=8, batch_size=4, seed=0)
    assert tuple(x.shape) == (4, 8)
    assert tuple(y.shape) == (4, 8)


def test_get_batch_dtype_is_long():
    data = _make_data(100)
    x, y = get_batch(data, block_size=8, batch_size=4, seed=0)
    assert x.dtype == torch.long
    assert y.dtype == torch.long


def test_get_batch_targets_are_shifted_by_one():
    # инвариант: y — это x, сдвинутый на один символ вперёд.
    # На data = [0,1,2,...] это значит y == x + 1 поэлементно.
    data = _make_data(100)
    x, y = get_batch(data, block_size=8, batch_size=4, seed=0)
    assert torch.equal(y, x + 1)


def test_get_batch_x_y_overlap():
    # более общая проверка сдвига: y[:, :-1] совпадает с x[:, 1:]
    torch.manual_seed(0)
    data = torch.randint(0, 5, (200,), dtype=torch.long)
    x, y = get_batch(data, block_size=10, batch_size=6, seed=123)
    assert torch.equal(y[:, :-1], x[:, 1:])


def test_get_batch_is_deterministic_with_seed():
    data = _make_data(100)
    x1, y1 = get_batch(data, block_size=8, batch_size=4, seed=42)
    x2, y2 = get_batch(data, block_size=8, batch_size=4, seed=42)
    assert torch.equal(x1, x2)
    assert torch.equal(y1, y2)


def test_get_batch_different_seed_differs():
    data = _make_data(100)
    x1, _ = get_batch(data, block_size=8, batch_size=8, seed=1)
    x2, _ = get_batch(data, block_size=8, batch_size=8, seed=2)
    # с подавляющей вероятностью батчи разные
    assert not torch.equal(x1, x2)


def test_get_batch_indices_within_bounds():
    # все значения попадают в исходный диапазон data (ничего не вылезло за край)
    data = _make_data(50)            # значения 0..49
    x, y = get_batch(data, block_size=8, batch_size=8, seed=7)
    assert int(x.min()) >= 0 and int(x.max()) <= 49
    assert int(y.min()) >= 0 and int(y.max()) <= 49
