# Эти тесты трогать не нужно — это эталон поведения (приёмочные тесты капстоуна).
#
# Майлстоун 2 (+ генерация из майлстоуна 4) — модель TransformerLM.
# Тесты детерминированы (сиды зафиксированы), крошечные, на CPU.

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


# ---------------------------------------------------------------------------
# forward: формы логитов
# ---------------------------------------------------------------------------

def test_forward_output_shape():
    vocab_size, block_size = 11, 8
    model = _tiny_model(vocab_size=vocab_size, block_size=block_size)
    B, T = 3, block_size
    idx = torch.randint(0, vocab_size, (B, T))
    logits = model(idx)
    assert tuple(logits.shape) == (B, T, vocab_size)


def test_forward_shorter_than_block_size():
    # T может быть меньше block_size — модель обязана это переварить.
    vocab_size, block_size = 7, 12
    model = _tiny_model(vocab_size=vocab_size, block_size=block_size)
    B, T = 2, 5
    idx = torch.randint(0, vocab_size, (B, T))
    logits = model(idx)
    assert tuple(logits.shape) == (B, T, vocab_size)


def test_forward_is_finite():
    vocab_size = 11
    model = _tiny_model(vocab_size=vocab_size)
    idx = torch.randint(0, vocab_size, (2, 8))
    logits = model(idx)
    assert torch.isfinite(logits).all()


# ---------------------------------------------------------------------------
# Причинность: будущее не влияет на прошлые позиции
# ---------------------------------------------------------------------------

def test_model_is_causal():
    # ОСОБЕННОСТЬ языковой модели: логиты позиции t не зависят от токенов > t.
    # Меняем ТОЛЬКО последний токен входа — логиты на всех предыдущих позициях
    # обязаны остаться прежними.
    vocab_size, block_size = 11, 8
    model = _tiny_model(vocab_size=vocab_size, block_size=block_size)
    model.eval()

    torch.manual_seed(1)
    idx = torch.randint(0, vocab_size, (1, block_size))
    with torch.no_grad():
        out_a = model(idx)

        idx2 = idx.clone()
        # портим только последнюю позицию (на другой токен)
        idx2[0, -1] = (idx[0, -1] + 1) % vocab_size
        out_b = model(idx2)

    # позиции 0..T-2 не должны измениться
    assert torch.allclose(out_a[:, :-1, :], out_b[:, :-1, :], atol=1e-6)


def test_model_changing_middle_token_affects_only_later():
    # меняем токен в середине -> логиты ДО него не меняются, ПОСЛЕ — меняются.
    vocab_size, block_size = 11, 8
    model = _tiny_model(vocab_size=vocab_size, block_size=block_size)
    model.eval()

    torch.manual_seed(2)
    idx = torch.randint(0, vocab_size, (1, block_size))
    pos = 4
    with torch.no_grad():
        out_a = model(idx)
        idx2 = idx.clone()
        idx2[0, pos] = (idx[0, pos] + 1) % vocab_size
        out_b = model(idx2)

    # до изменённой позиции — без изменений
    assert torch.allclose(out_a[:, :pos, :], out_b[:, :pos, :], atol=1e-6)
    # на изменённой позиции (и/или после) что-то поменялось
    assert not torch.allclose(out_a[:, pos:, :], out_b[:, pos:, :], atol=1e-6)


# ---------------------------------------------------------------------------
# Позиционные эмбеддинги действительно используются
# ---------------------------------------------------------------------------

def test_position_matters():
    # Если позиция учитывается, то один и тот же токен на разных местах
    # в общем случае даёт разные логиты. Берём повторяющийся токен в двух
    # позициях и проверяем, что логиты по позициям не идентичны.
    vocab_size, block_size = 11, 8
    model = _tiny_model(vocab_size=vocab_size, block_size=block_size)
    model.eval()
    idx = torch.zeros(1, block_size, dtype=torch.long)   # один и тот же токен везде
    with torch.no_grad():
        logits = model(idx)                              # (1, T, vocab)
    # логиты на позиции 0 и на позиции T-1 не должны полностью совпадать
    assert not torch.allclose(logits[0, 0, :], logits[0, -1, :], atol=1e-5)


# ---------------------------------------------------------------------------
# generate: формы, границы, детерминизм при фиксированном сиде
# ---------------------------------------------------------------------------

def test_generate_output_shape():
    vocab_size, block_size = 11, 8
    model = _tiny_model(vocab_size=vocab_size, block_size=block_size)
    model.eval()
    idx = torch.randint(0, vocab_size, (1, 3))
    out = model.generate(idx, max_new_tokens=10)
    assert tuple(out.shape) == (1, 3 + 10)


def test_generate_preserves_prefix():
    # затравка должна остаться в начале результата без изменений
    vocab_size, block_size = 11, 8
    model = _tiny_model(vocab_size=vocab_size, block_size=block_size)
    model.eval()
    idx = torch.randint(0, vocab_size, (1, 4))
    out = model.generate(idx, max_new_tokens=6)
    assert torch.equal(out[:, :4], idx)


def test_generate_tokens_in_vocab_range():
    vocab_size, block_size = 11, 8
    model = _tiny_model(vocab_size=vocab_size, block_size=block_size)
    model.eval()
    idx = torch.randint(0, vocab_size, (1, 2))
    out = model.generate(idx, max_new_tokens=20)
    assert int(out.min()) >= 0
    assert int(out.max()) < vocab_size


def test_generate_handles_context_longer_than_block_size():
    # затравка длиннее block_size -> generate не должен падать (контекст обрезается)
    vocab_size, block_size = 11, 6
    model = _tiny_model(vocab_size=vocab_size, block_size=block_size)
    model.eval()
    idx = torch.randint(0, vocab_size, (1, 10))     # 10 > block_size=6
    out = model.generate(idx, max_new_tokens=5)
    assert tuple(out.shape) == (1, 15)


def test_generate_is_deterministic_with_seed():
    # при одинаковом сиде до вызова generate выдаёт одну и ту же выборку
    vocab_size, block_size = 11, 8
    model = _tiny_model(vocab_size=vocab_size, block_size=block_size)
    model.eval()
    idx = torch.randint(0, vocab_size, (1, 3))

    torch.manual_seed(123)
    out1 = model.generate(idx.clone(), max_new_tokens=15)
    torch.manual_seed(123)
    out2 = model.generate(idx.clone(), max_new_tokens=15)
    assert torch.equal(out1, out2)
