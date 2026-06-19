# Эти тесты трогать не нужно — это эталон поведения.
#
# Модуль 14 — Последовательности: эмбеддинги и RNN. Тесты детерминированы
# (сиды зафиксированы), крошечные и идут на CPU. Никаких загрузок данных —
# игрушечные последовательности генерируются прямо здесь.

import numpy as np  # noqa: F401  (импорт обязателен по стилю курса; используется в part-проверках)
import torch
import torch.nn as nn

from seqmodels import CharRNN, train_step


# ---------------------------------------------------------------------------
# CharRNN: устройство модели и формы выхода
# ---------------------------------------------------------------------------

def test_charrnn_is_nn_module():
    torch.manual_seed(0)
    model = CharRNN(vocab_size=5, embed_dim=4, hidden_size=6)
    assert isinstance(model, nn.Module)


def test_charrnn_has_expected_layers():
    # три слоя нужных типов: Embedding -> RNN -> Linear
    torch.manual_seed(0)
    model = CharRNN(vocab_size=7, embed_dim=3, hidden_size=8)
    layer_types = {type(m) for m in model.modules()}
    assert nn.Embedding in layer_types
    assert nn.RNN in layer_types
    assert nn.Linear in layer_types


def test_charrnn_output_shape():
    # вход (B, T) целых -> выход логитов (B, T, vocab_size)
    torch.manual_seed(0)
    vocab_size, embed_dim, hidden_size = 5, 4, 6
    B, T = 3, 10
    model = CharRNN(vocab_size, embed_dim, hidden_size)
    x = torch.randint(0, vocab_size, (B, T))   # целые индексы
    logits = model(x)
    assert tuple(logits.shape) == (B, T, vocab_size)


def test_charrnn_output_shape_other_sizes():
    # форма выхода должна зависеть только от (B, T, vocab_size)
    torch.manual_seed(1)
    vocab_size, embed_dim, hidden_size = 11, 7, 5
    B, T = 2, 4
    model = CharRNN(vocab_size, embed_dim, hidden_size)
    x = torch.randint(0, vocab_size, (B, T))
    logits = model(x)
    assert tuple(logits.shape) == (B, T, vocab_size)


def test_charrnn_output_is_float_tensor():
    # логиты — вещественный тензор (от них считают cross-entropy)
    torch.manual_seed(2)
    model = CharRNN(vocab_size=6, embed_dim=3, hidden_size=4)
    x = torch.randint(0, 6, (2, 5))
    logits = model(x)
    assert logits.dtype == torch.float32


def test_charrnn_batch_dim_independent():
    # одна и та же строка в батче должна давать одинаковые логиты
    # (значит RNN корректно идёт по батчу как по независимым примерам)
    torch.manual_seed(3)
    vocab_size = 6
    model = CharRNN(vocab_size, embed_dim=4, hidden_size=5)
    model.eval()
    row = torch.randint(0, vocab_size, (1, 7))
    x = torch.cat([row, row], dim=0)          # (2, 7) — две одинаковые строки
    logits = model(x)
    assert torch.allclose(logits[0], logits[1], atol=1e-6)


# ---------------------------------------------------------------------------
# train_step: возвращает число и реально делает шаг (loss меняется, параметры тоже)
# ---------------------------------------------------------------------------

def _toy_batch(vocab_size, B, T, seed=0):
    """Игрушечный батч: X — случайные индексы, Y — сдвиг X на 1 шаг влево.

    Сдвиг на 1 = «предскажи следующий символ»: Y[:, t] это X[:, t+1],
    а последний столбец заворачиваем циклически (любой валидный индекс подойдёт).
    """
    g = torch.Generator().manual_seed(seed)
    X = torch.randint(0, vocab_size, (B, T), generator=g)
    Y = torch.roll(X, shifts=-1, dims=1)
    return X, Y


def test_train_step_returns_float():
    torch.manual_seed(0)
    vocab_size = 5
    model = CharRNN(vocab_size, embed_dim=4, hidden_size=6)
    optimizer = torch.optim.Adam(model.parameters(), lr=0.01)
    loss_fn = nn.CrossEntropyLoss()
    X, Y = _toy_batch(vocab_size, B=3, T=8, seed=0)
    loss = train_step(model, optimizer, loss_fn, X, Y)
    assert isinstance(loss, float)
    assert loss > 0.0


def test_train_step_updates_parameters():
    # после шага хотя бы один параметр должен измениться
    torch.manual_seed(0)
    vocab_size = 5
    model = CharRNN(vocab_size, embed_dim=4, hidden_size=6)
    optimizer = torch.optim.Adam(model.parameters(), lr=0.05)
    loss_fn = nn.CrossEntropyLoss()
    X, Y = _toy_batch(vocab_size, B=4, T=8, seed=1)

    before = [p.detach().clone() for p in model.parameters()]
    train_step(model, optimizer, loss_fn, X, Y)
    after = list(model.parameters())

    changed = any(not torch.allclose(b, a) for b, a in zip(before, after))
    assert changed


def test_train_step_matches_manual_loss_value():
    # возвращённый loss должен совпасть с cross-entropy, посчитанной вручную
    # на тех же параметрах ДО шага
    torch.manual_seed(0)
    vocab_size = 6
    model = CharRNN(vocab_size, embed_dim=4, hidden_size=5)
    optimizer = torch.optim.SGD(model.parameters(), lr=0.0)  # lr=0: параметры не сдвинутся
    loss_fn = nn.CrossEntropyLoss()
    X, Y = _toy_batch(vocab_size, B=2, T=6, seed=2)

    # эталон: forward + cross-entropy на схлопнутых формах, без шага
    with torch.no_grad():
        logits = model(X)
        expected = loss_fn(logits.reshape(-1, vocab_size), Y.reshape(-1)).item()

    got = train_step(model, optimizer, loss_fn, X, Y)
    assert abs(got - expected) < 1e-5


# ---------------------------------------------------------------------------
# Главное: на крошечной seeded последовательности обучение снижает loss.
# Сеть учится запоминать ОДНУ фиксированную строку (next-char prediction).
# Проверяем: итоговый loss < 0.5 * начальный.
# ---------------------------------------------------------------------------

def test_training_reduces_loss_on_memorized_sequence():
    torch.manual_seed(0)
    vocab_size = 8

    # одна фиксированная последовательность (B=1), задача — выучить её наизусть
    B, T = 1, 20
    g = torch.Generator().manual_seed(123)
    seq = torch.randint(0, vocab_size, (B, T), generator=g)
    X = seq
    Y = torch.roll(seq, shifts=-1, dims=1)

    model = CharRNN(vocab_size, embed_dim=8, hidden_size=16)
    optimizer = torch.optim.Adam(model.parameters(), lr=0.01)
    loss_fn = nn.CrossEntropyLoss()

    initial_loss = train_step(model, optimizer, loss_fn, X, Y)
    final_loss = initial_loss
    for _ in range(200):
        final_loss = train_step(model, optimizer, loss_fn, X, Y)

    assert final_loss < 0.5 * initial_loss


def test_training_reduces_loss_on_small_batch():
    # тот же эффект на батче из нескольких коротких строк
    torch.manual_seed(0)
    vocab_size = 6
    B, T = 4, 12
    g = torch.Generator().manual_seed(7)
    seqs = torch.randint(0, vocab_size, (B, T), generator=g)
    X = seqs
    Y = torch.roll(seqs, shifts=-1, dims=1)

    model = CharRNN(vocab_size, embed_dim=6, hidden_size=16)
    optimizer = torch.optim.Adam(model.parameters(), lr=0.01)
    loss_fn = nn.CrossEntropyLoss()

    initial_loss = train_step(model, optimizer, loss_fn, X, Y)
    final_loss = initial_loss
    for _ in range(250):
        final_loss = train_step(model, optimizer, loss_fn, X, Y)

    assert final_loss < 0.5 * initial_loss
