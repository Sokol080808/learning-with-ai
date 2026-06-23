# Эти тесты трогать не нужно — это приёмочные тесты управления выборкой
# (майлстоун 4: temperature и top-k в generate).
#
# Все тесты — на ИНВАРИАНТАХ (внешний оракул не нужен): крошечная seeded модель
# на CPU, проверяем контракт, не прибивая внутренности RNG. Стиль повторяет
# tests/test_model.py.

import torch

from nanolm.model import TransformerLM


def _tiny_model(vocab_size=11, block_size=8, d_model=16, n_heads=4, n_layers=2):
    torch.manual_seed(0)
    return TransformerLM(
        vocab_size=vocab_size,
        block_size=block_size,
        d_model=d_model,
        n_heads=n_heads,
        n_layers=n_layers,
    )


def _greedy_next_id(model, idx):
    """Эталонный argmax следующего токена для контекста idx (как делает forward)."""
    idx_cond = idx[:, -model.block_size:]
    with torch.no_grad():
        logits = model(idx_cond)[:, -1, :]            # (B, vocab)
    return torch.argmax(logits, dim=-1, keepdim=True)  # (B, 1)


# ---------------------------------------------------------------------------
# BACKWARD-COMPAT: дефолты обязаны воспроизводить прежнее поведение бит-в-бит
# ---------------------------------------------------------------------------

def test_defaults_match_plain_generate():
    # generate(idx, n) и generate(idx, n, temperature=1.0, top_k=None) под
    # одинаковым пре-сидом обязаны совпасть бит-в-бит: дефолты — настоящий no-op.
    vocab_size, block_size = 11, 8
    model = _tiny_model(vocab_size=vocab_size, block_size=block_size)
    model.eval()
    idx = torch.randint(0, vocab_size, (1, 3))

    torch.manual_seed(123)
    out_plain = model.generate(idx.clone(), max_new_tokens=15)
    torch.manual_seed(123)
    out_explicit = model.generate(idx.clone(), max_new_tokens=15, temperature=1.0, top_k=None)
    assert torch.equal(out_plain, out_explicit)


def test_top_k_equal_vocab_is_noop():
    # top_k = vocab_size ничего не обрезает (все логиты проходят), поэтому при
    # T=1.0 результат совпадает с дефолтным generate под тем же сидом.
    vocab_size, block_size = 11, 8
    model = _tiny_model(vocab_size=vocab_size, block_size=block_size)
    model.eval()
    idx = torch.randint(0, vocab_size, (1, 3))

    torch.manual_seed(7)
    out_plain = model.generate(idx.clone(), max_new_tokens=12)
    torch.manual_seed(7)
    out_full_k = model.generate(idx.clone(), max_new_tokens=12, top_k=vocab_size)
    assert torch.equal(out_plain, out_full_k)


# ---------------------------------------------------------------------------
# TOP_K=1 — это жадный argmax: детерминирован и независим от RNG
# ---------------------------------------------------------------------------

def test_top_k_1_is_deterministic_regardless_of_seed():
    # При top_k=1 в каждой строке остаётся ровно один не-маскированный логит,
    # поэтому multinomial вынужден его выбрать. Два запуска с РАЗНЫМИ пре-сидами
    # обязаны дать идентичный вывод.
    vocab_size, block_size = 11, 8
    model = _tiny_model(vocab_size=vocab_size, block_size=block_size)
    model.eval()
    idx = torch.randint(0, vocab_size, (1, 3))

    torch.manual_seed(1)
    out_a = model.generate(idx.clone(), max_new_tokens=15, top_k=1)
    torch.manual_seed(999)
    out_b = model.generate(idx.clone(), max_new_tokens=15, top_k=1)
    assert torch.equal(out_a, out_b)


def test_top_k_1_matches_argmax_step_by_step():
    # Каждый дописанный top_k=1 токен обязан равняться argmax логитов последней
    # позиции для текущего контекста (пересчитываем через forward).
    vocab_size, block_size = 11, 8
    model = _tiny_model(vocab_size=vocab_size, block_size=block_size)
    model.eval()
    idx = torch.randint(0, vocab_size, (1, 3))

    cur = idx.clone()
    for _ in range(10):
        expected = _greedy_next_id(model, cur)                 # (1, 1)
        cur = model.generate(cur, max_new_tokens=1, top_k=1)   # дописали один токен
        assert torch.equal(cur[:, -1:], expected)


# ---------------------------------------------------------------------------
# TOP_K ограничивает носитель: выбранный токен всегда в top-k argmax
# ---------------------------------------------------------------------------

def test_top_k_restricts_support_to_top_k_ids():
    # На каждом шаге сэмплированный токен обязан лежать среди k наибольших
    # логитов текущего контекста (torch.topk(...).indices).
    vocab_size, block_size, k = 13, 8, 3
    model = _tiny_model(vocab_size=vocab_size, block_size=block_size)
    model.eval()
    idx = torch.randint(0, vocab_size, (1, 3))

    torch.manual_seed(42)
    cur = idx.clone()
    for _ in range(20):
        idx_cond = cur[:, -model.block_size:]
        with torch.no_grad():
            logits = model(idx_cond)[:, -1, :]                 # (1, vocab)
        allowed = torch.topk(logits, k, dim=-1).indices[0]     # (k,)
        cur = model.generate(cur, max_new_tokens=1, top_k=k)
        chosen = int(cur[0, -1])
        assert chosen in set(allowed.tolist())


# ---------------------------------------------------------------------------
# TEMPERATURE -> 0+ сходится к жадной (top_k=1) последовательности
# ---------------------------------------------------------------------------

def test_temperature_near_zero_converges_to_greedy():
    # На общем пределе temperature->0 и top_k=1 согласованы: оба дают argmax.
    vocab_size, block_size = 11, 8
    model = _tiny_model(vocab_size=vocab_size, block_size=block_size)
    model.eval()
    idx = torch.randint(0, vocab_size, (1, 3))

    greedy = model.generate(idx.clone(), max_new_tokens=15, top_k=1)
    torch.manual_seed(5)
    cold = model.generate(idx.clone(), max_new_tokens=15, temperature=1e-4)
    assert torch.equal(greedy, cold)


# ---------------------------------------------------------------------------
# VALIDITY: форма и диапазон сохраняются при любых temperature/top_k
# ---------------------------------------------------------------------------

def test_sampling_preserves_shape_and_range():
    vocab_size, block_size = 11, 8
    model = _tiny_model(vocab_size=vocab_size, block_size=block_size)
    model.eval()
    idx = torch.randint(0, vocab_size, (2, 3))

    combos = [
        {"temperature": 0.5},
        {"temperature": 2.0},
        {"top_k": 1},
        {"top_k": 5},
        {"temperature": 0.8, "top_k": 3},
    ]
    for kw in combos:
        torch.manual_seed(0)
        out = model.generate(idx.clone(), max_new_tokens=10, **kw)
        assert tuple(out.shape) == (2, 3 + 10), kw
        assert int(out.min()) >= 0, kw
        assert int(out.max()) < vocab_size, kw


def test_sampling_preserves_prefix():
    # затравка остаётся в начале при любой настройке выборки
    vocab_size, block_size = 11, 8
    model = _tiny_model(vocab_size=vocab_size, block_size=block_size)
    model.eval()
    idx = torch.randint(0, vocab_size, (1, 4))

    torch.manual_seed(0)
    out = model.generate(idx.clone(), max_new_tokens=6, temperature=1.3, top_k=4)
    assert torch.equal(out[:, :4], idx)


def test_top_k_larger_than_vocab_is_clamped():
    # k > vocab_size не должен падать (k ограничивается размером словаря).
    vocab_size, block_size = 7, 8
    model = _tiny_model(vocab_size=vocab_size, block_size=block_size)
    model.eval()
    idx = torch.randint(0, vocab_size, (1, 3))

    torch.manual_seed(0)
    out = model.generate(idx.clone(), max_new_tokens=8, top_k=999)
    assert tuple(out.shape) == (1, 3 + 8)
    assert int(out.min()) >= 0
    assert int(out.max()) < vocab_size
