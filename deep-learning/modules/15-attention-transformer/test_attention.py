# Эти тесты трогать не нужно — это эталон поведения.
#
# Модуль 15 — Внимание и трансформеры. Тесты детерминированы (сиды зафиксированы),
# крошечные и идут на CPU.

import math

import pytest
import torch
import torch.nn as nn

from attention import (
    scaled_dot_product_attention,
    causal_mask,
    MultiHeadAttention,
    TransformerBlock,
    sinusoidal_positional_encoding,
    LearnedPositionalEncoding,
)


# ---------------------------------------------------------------------------
# scaled_dot_product_attention: формы, нормировка весов, сверка с эталоном
# ---------------------------------------------------------------------------

def test_sdpa_output_and_weight_shapes():
    torch.manual_seed(0)
    q = torch.randn(1, 3, 4)        # (B=1, T_q=3, d_k=4)
    k = torch.randn(1, 5, 4)        # (B=1, T_k=5, d_k=4)
    v = torch.randn(1, 5, 6)        # (B=1, T_k=5, d_v=6)
    out, attn = scaled_dot_product_attention(q, k, v)
    assert tuple(out.shape) == (1, 3, 6)
    assert tuple(attn.shape) == (1, 3, 5)


def test_sdpa_weights_are_probabilities():
    # каждая строка весов — распределение: неотрицательна и суммируется в 1
    torch.manual_seed(1)
    q = torch.randn(2, 4, 8)
    k = torch.randn(2, 4, 8)
    v = torch.randn(2, 4, 8)
    _, attn = scaled_dot_product_attention(q, k, v)
    assert torch.all(attn >= 0)
    row_sums = attn.sum(dim=-1)
    assert torch.allclose(row_sums, torch.ones_like(row_sums), atol=1e-6)


def test_sdpa_uses_sqrt_dk_scaling():
    # сверка с эталонной формулой softmax(q k^T / sqrt(d_k)) @ v
    torch.manual_seed(2)
    d_k = 16
    q = torch.randn(1, 3, d_k)
    k = torch.randn(1, 3, d_k)
    v = torch.randn(1, 3, 5)
    out, attn = scaled_dot_product_attention(q, k, v)

    scores = (q @ k.transpose(-2, -1)) / math.sqrt(d_k)
    ref_attn = torch.softmax(scores, dim=-1)
    ref_out = ref_attn @ v

    assert torch.allclose(attn, ref_attn, atol=1e-6)
    assert torch.allclose(out, ref_out, atol=1e-6)


def test_sdpa_without_scaling_would_differ():
    # тот же вход без деления на sqrt(d_k) дал бы ДРУГИЕ веса —
    # значит нормировка действительно применяется (не случайно совпадает)
    torch.manual_seed(3)
    d_k = 32
    q = torch.randn(1, 4, d_k)
    k = torch.randn(1, 4, d_k)
    v = torch.randn(1, 4, d_k)
    _, attn = scaled_dot_product_attention(q, k, v)

    scores_unscaled = q @ k.transpose(-2, -1)          # БЕЗ деления
    attn_unscaled = torch.softmax(scores_unscaled, dim=-1)
    assert not torch.allclose(attn, attn_unscaled, atol=1e-4)


def test_sdpa_uniform_when_keys_equal():
    # если все ключи одинаковы, все scores в строке равны -> веса равномерные
    q = torch.randn(1, 2, 4)
    k = torch.ones(1, 3, 4)        # три одинаковых ключа
    v = torch.randn(1, 3, 4)
    _, attn = scaled_dot_product_attention(q, k, v)
    expected = torch.full((1, 2, 3), 1.0 / 3.0)
    assert torch.allclose(attn, expected, atol=1e-6)


# ---------------------------------------------------------------------------
# causal_mask: форма, тип, треугольная структура
# ---------------------------------------------------------------------------

def test_causal_mask_shape_and_dtype():
    m = causal_mask(4)
    assert tuple(m.shape) == (4, 4)
    assert m.dtype == torch.bool


def test_causal_mask_values():
    # True == запрет; запрещаем строго будущее (j > i)
    m = causal_mask(3)
    expected = torch.tensor(
        [[False, True, True],
         [False, False, True],
         [False, False, False]]
    )
    assert torch.equal(m, expected)


def test_causal_mask_diagonal_allowed():
    # текущую и прошлые позиции (j <= i) смотреть РАЗРЕШЕНО -> False
    m = causal_mask(5)
    for i in range(5):
        for j in range(5):
            if j <= i:
                assert m[i, j] == False  # noqa: E712
            else:
                assert m[i, j] == True   # noqa: E712


# ---------------------------------------------------------------------------
# Внимание под causal-маской: будущее не подсматривается (веса ~0)
# ---------------------------------------------------------------------------

def test_causal_attention_zero_weight_on_future():
    # ОСОБЕННОСТЬ модуля: при causal-маске внимание к будущим позициям ~ 0.
    torch.manual_seed(0)
    T, d = 6, 8
    q = torch.randn(1, T, d)
    k = torch.randn(1, T, d)
    v = torch.randn(1, T, d)
    mask = causal_mask(T)

    _, attn = scaled_dot_product_attention(q, k, v, mask)
    # attn: (1, T, T). Вес на позицию из будущего (j > i) должен быть ~0.
    attn0 = attn[0]
    for i in range(T):
        for j in range(T):
            if j > i:
                assert attn0[i, j].item() < 1e-6
    # и при этом строки всё ещё суммируются в 1 (масса ушла в прошлое)
    assert torch.allclose(attn0.sum(dim=-1), torch.ones(T), atol=1e-6)


def test_causal_first_row_attends_only_to_itself():
    # позиция 0 не видит ничего, кроме себя -> весь вес на столбце 0
    torch.manual_seed(1)
    T, d = 4, 8
    q = torch.randn(1, T, d)
    k = torch.randn(1, T, d)
    v = torch.randn(1, T, d)
    _, attn = scaled_dot_product_attention(q, k, v, causal_mask(T))
    assert attn[0, 0, 0].item() > 1.0 - 1e-6
    assert torch.allclose(attn[0, 0, 1:], torch.zeros(T - 1), atol=1e-6)


# ---------------------------------------------------------------------------
# MultiHeadAttention: форма выхода, делимость, причинность на уровне модуля
# ---------------------------------------------------------------------------

def test_mha_output_shape():
    torch.manual_seed(0)
    mha = MultiHeadAttention(d_model=16, n_heads=4)
    x = torch.randn(2, 5, 16)          # (B=2, T=5, d_model=16)
    out = mha(x)
    assert tuple(out.shape) == (2, 5, 16)


def test_mha_single_head_matches_sdpa_structure():
    # с одной головой выход MHA = w_o( sdpa(w_q x, w_k x, w_v x) )
    torch.manual_seed(0)
    mha = MultiHeadAttention(d_model=8, n_heads=1)
    x = torch.randn(1, 4, 8)
    out = mha(x)

    q = mha.w_q(x)
    k = mha.w_k(x)
    v = mha.w_v(x)
    sdpa_out, _ = scaled_dot_product_attention(q, k, v, causal_mask(4))
    ref = mha.w_o(sdpa_out)
    assert torch.allclose(out, ref, atol=1e-5)


def test_mha_is_causal_future_does_not_affect_past():
    # причинность на уровне модуля: меняем ТОЛЬКО последний токен входа —
    # выход на всех предыдущих позициях не должен измениться.
    torch.manual_seed(0)
    mha = MultiHeadAttention(d_model=16, n_heads=4)
    x = torch.randn(1, 5, 16)
    out_a = mha(x)

    x2 = x.clone()
    x2[:, -1, :] = torch.randn(16)     # портим только будущее (последнюю позицию)
    out_b = mha(x2)

    # позиции 0..3 не зависят от позиции 4 -> совпадают
    assert torch.allclose(out_a[:, :-1, :], out_b[:, :-1, :], atol=1e-6)


# ---------------------------------------------------------------------------
# TransformerBlock: форма, работа на seeded входе, обучаемость
# ---------------------------------------------------------------------------

def test_transformer_block_output_shape():
    torch.manual_seed(0)
    block = TransformerBlock(d_model=16, n_heads=4)
    x = torch.randn(2, 7, 16)
    out = block(x)
    assert tuple(out.shape) == (2, 7, 16)


def test_transformer_block_forward_runs_and_is_finite():
    # forward на seeded входе отрабатывает и не выдаёт nan/inf
    torch.manual_seed(123)
    block = TransformerBlock(d_model=8, n_heads=2)
    x = torch.randn(3, 6, 8)
    out = block(x)
    assert tuple(out.shape) == (3, 6, 8)
    assert torch.isfinite(out).all()


def test_transformer_block_is_causal():
    # блок целиком остаётся причинным: будущий токен не влияет на прошлые позиции
    torch.manual_seed(0)
    block = TransformerBlock(d_model=16, n_heads=4)
    x = torch.randn(1, 5, 16)
    out_a = block(x)
    x2 = x.clone()
    x2[:, -1, :] = torch.randn(16)
    out_b = block(x2)
    assert torch.allclose(out_a[:, :-1, :], out_b[:, :-1, :], atol=1e-6)


def test_transformer_block_trains_reduces_loss():
    # стопка блоков должна уметь учиться: на игрушечной задаче
    # «предскажи следующий вход» итоговый loss заметно меньше начального.
    torch.manual_seed(0)
    B, T, d_model = 4, 8, 16
    block = TransformerBlock(d_model=d_model, n_heads=4)
    head = nn.Linear(d_model, d_model)

    x = torch.randn(B, T, d_model)
    # цель: для каждой позиции t (кроме последней) предсказать вход на t+1.
    target = x[:, 1:, :]               # (B, T-1, d_model)

    params = list(block.parameters()) + list(head.parameters())
    opt = torch.optim.Adam(params, lr=1e-2)

    def compute_loss():
        h = block(x)                   # (B, T, d_model), причинно
        pred = head(h)[:, :-1, :]      # предсказание для t+1 по позиции t
        return ((pred - target) ** 2).mean()

    initial_loss = compute_loss().item()
    for _ in range(120):
        opt.zero_grad()
        loss = compute_loss()
        loss.backward()
        opt.step()
    final_loss = compute_loss().item()

    assert final_loss < 0.5 * initial_loss


# ---------------------------------------------------------------------------
# Позиционное кодирование: форма, формула sin/cos, обучаемость
# ---------------------------------------------------------------------------

def test_sinusoidal_pe_shape():
    pe = sinusoidal_positional_encoding(5, 8)
    assert tuple(pe.shape) == (5, 8)


def test_sinusoidal_pe_matches_formula_hand_checked():
    # сверка конкретных (pos, 2i) с замкнутой формулой из статьи
    T, d_model = 6, 8
    pe = sinusoidal_positional_encoding(T, d_model)
    for pos in [0, 1, 3, 5]:
        for i2 in [0, 2, 4, 6]:           # 2i: чётные каналы
            i = i2 // 2
            denom = 10000.0 ** (i2 / d_model)
            expected_sin = math.sin(pos / denom)
            expected_cos = math.cos(pos / denom)
            assert torch.isclose(pe[pos, i2], torch.tensor(expected_sin), atol=1e-6)
            assert torch.isclose(pe[pos, i2 + 1], torch.tensor(expected_cos), atol=1e-6)


def test_sinusoidal_pe_row_zero_is_sin0_cos0():
    # pos=0 -> синусы=0, косинусы=1 -> [0,1,0,1,...]
    pe = sinusoidal_positional_encoding(3, 6)
    expected_row0 = torch.tensor([0.0, 1.0, 0.0, 1.0, 0.0, 1.0])
    assert torch.allclose(pe[0], expected_row0, atol=1e-6)


def test_sinusoidal_pe_is_not_trainable():
    # синусоидальный код — константа: тензор без grad
    pe = sinusoidal_positional_encoding(4, 8)
    assert pe.requires_grad is False


def test_sinusoidal_pe_odd_d_model_raises():
    # нечётный d_model нельзя разложить по парам sin/cos
    with pytest.raises(ValueError):
        sinusoidal_positional_encoding(4, 7)


def test_sinusoidal_pe_broadcasts_onto_batch():
    # x + PE(T, d_model) сохраняет форму (B, T, d_model)
    torch.manual_seed(0)
    B, T, d_model = 2, 5, 8
    x = torch.randn(B, T, d_model)
    pe = sinusoidal_positional_encoding(T, d_model)
    out = x + pe
    assert tuple(out.shape) == (B, T, d_model)


def test_learned_pe_shape():
    pe = LearnedPositionalEncoding(max_T=16, d_model=8)
    out = pe(5)
    assert tuple(out.shape) == (5, 8)


def test_learned_pe_is_trainable_changes_after_step():
    # обучаемый код: после шага Adam значения эмбеддинга меняются
    torch.manual_seed(0)
    pe = LearnedPositionalEncoding(max_T=16, d_model=8)
    before = pe.emb.weight.detach().clone()
    opt = torch.optim.Adam(pe.parameters(), lr=1e-1)
    out = pe(5)
    loss = (out ** 2).mean()
    opt.zero_grad()
    loss.backward()
    opt.step()
    after = pe.emb.weight.detach().clone()
    assert not torch.allclose(before, after, atol=1e-6)


def test_learned_and_sinusoidal_differ():
    # два способа дают РАЗНЫЕ значения для одного (T, d_model)
    torch.manual_seed(0)
    T, d_model = 5, 8
    sin_pe = sinusoidal_positional_encoding(T, d_model)
    learned = LearnedPositionalEncoding(max_T=16, d_model=d_model)(T)
    assert not torch.allclose(sin_pe, learned, atol=1e-3)
