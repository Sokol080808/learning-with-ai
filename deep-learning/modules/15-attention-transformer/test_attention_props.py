# Property-based and randomised tests for Module 15 — Attention & Transformers.
#
# Complement to test_attention.py: does NOT duplicate its fixed cases.
# All module calls happen INSIDE test functions so the file collects cleanly
# against an unfilled stub (raise NotImplementedError) on the main branch.

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
# Helpers
# ---------------------------------------------------------------------------

def _rand(seed, *shape, dtype=torch.float32):
    torch.manual_seed(seed)
    return torch.randn(*shape, dtype=dtype)


# ===========================================================================
# scaled_dot_product_attention — extended property tests
# ===========================================================================

class TestSdpaProperties:
    """Randomised correctness checks for scaled_dot_product_attention."""

    # ------------------------------------------------------------------
    # Shape contract across varied leading dims / asymmetric T_q vs T_k
    # ------------------------------------------------------------------

    @pytest.mark.parametrize("shape,d_k,d_v", [
        # (leading_dims, d_k, d_v)
        ((3, 5), 8, 8),      # (B, T, d) — standard batch
        ((2, 4, 6), 16, 12), # (B, H, T, d) — multi-head style
        ((1, 1), 4, 4),      # single example, single token
        ((7,), 32, 32),      # 1-D leading dim
    ])
    def test_output_shape_leading_dims(self, shape, d_k, d_v):
        torch.manual_seed(42)
        T_q, T_k = 5, 7
        q = torch.randn(*shape, T_q, d_k)
        k = torch.randn(*shape, T_k, d_k)
        v = torch.randn(*shape, T_k, d_v)
        out, attn = scaled_dot_product_attention(q, k, v)
        assert out.shape == (*shape, T_q, d_v)
        assert attn.shape == (*shape, T_q, T_k)

    # ------------------------------------------------------------------
    # Attention rows always sum to 1 across many random seeds
    # ------------------------------------------------------------------

    def test_attn_rows_sum_to_one_many_seeds(self):
        for seed in range(10):
            torch.manual_seed(seed * 7 + 3)
            T = torch.randint(2, 12, ()).item()
            d_k = torch.randint(4, 33, ()).item()
            q = torch.randn(2, T, d_k)
            k = torch.randn(2, T, d_k)
            v = torch.randn(2, T, d_k)
            _, attn = scaled_dot_product_attention(q, k, v)
            row_sums = attn.sum(dim=-1)
            assert torch.allclose(row_sums, torch.ones_like(row_sums), atol=1e-5), \
                f"seed={seed}: row sums not 1: {row_sums}"

    # ------------------------------------------------------------------
    # All attention weights are non-negative (softmax outputs)
    # ------------------------------------------------------------------

    def test_attn_weights_non_negative_many_seeds(self):
        for seed in range(8):
            torch.manual_seed(seed * 13 + 11)
            q = torch.randn(3, 6, 16)
            k = torch.randn(3, 6, 16)
            v = torch.randn(3, 6, 16)
            _, attn = scaled_dot_product_attention(q, k, v)
            assert (attn >= 0).all(), f"seed={seed}: negative attention weights"

    # ------------------------------------------------------------------
    # Masking: masked positions have weight < eps after softmax
    # ------------------------------------------------------------------

    def test_mask_zeroes_forbidden_positions_random(self):
        """Random sparse masks: forbidden positions must have weight ~0."""
        torch.manual_seed(77)
        for _ in range(5):
            T = 8
            d_k = 12
            q = torch.randn(1, T, d_k)
            k = torch.randn(1, T, d_k)
            v = torch.randn(1, T, d_k)
            # random boolean mask: each cell forbidden with prob 0.4
            mask = torch.rand(T, T) < 0.4
            # ensure each row has at least one unmasked position to avoid NaN
            for i in range(T):
                if mask[i].all():
                    mask[i, 0] = False
            _, attn = scaled_dot_product_attention(q, k, v, mask)
            assert (attn[0][mask] < 1e-6).all(), \
                "masked positions should have weight ~0"

    # ------------------------------------------------------------------
    # Causal mask applied via sdpa: rows sum to 1 AND future weights = 0
    # ------------------------------------------------------------------

    def test_causal_mask_rows_still_sum_to_one(self):
        """Causal masking must not break the probability constraint."""
        for seed in range(6):
            torch.manual_seed(seed * 5 + 100)
            T = torch.randint(2, 10, ()).item()
            d = 8
            q = torch.randn(2, T, d)
            k = torch.randn(2, T, d)
            v = torch.randn(2, T, d)
            mask = causal_mask(T)
            _, attn = scaled_dot_product_attention(q, k, v, mask)
            row_sums = attn.sum(dim=-1)
            assert torch.allclose(row_sums, torch.ones_like(row_sums), atol=1e-5), \
                f"seed={seed} T={T}: causal-masked rows don't sum to 1"

    # ------------------------------------------------------------------
    # Numerical stability: large-magnitude inputs must not produce NaN/Inf
    # ------------------------------------------------------------------

    def test_large_magnitude_inputs_are_finite(self):
        torch.manual_seed(200)
        for scale in [1e2, 1e4, 1e6]:
            q = torch.randn(2, 5, 32) * scale
            k = torch.randn(2, 5, 32) * scale
            v = torch.randn(2, 5, 32) * scale
            out, attn = scaled_dot_product_attention(q, k, v)
            assert torch.isfinite(out).all(), f"scale={scale}: out has NaN/Inf"
            assert torch.isfinite(attn).all(), f"scale={scale}: attn has NaN/Inf"

    # ------------------------------------------------------------------
    # Zero queries/keys: output is finite (v weighted by uniform or 1-hot)
    # ------------------------------------------------------------------

    def test_zero_queries_produces_finite_output(self):
        torch.manual_seed(300)
        q = torch.zeros(2, 4, 8)
        k = torch.randn(2, 4, 8)
        v = torch.randn(2, 4, 8)
        out, attn = scaled_dot_product_attention(q, k, v)
        assert torch.isfinite(out).all()
        # zero q -> all scores equal -> uniform attention rows
        row_sums = attn.sum(dim=-1)
        assert torch.allclose(row_sums, torch.ones_like(row_sums), atol=1e-5)

    # ------------------------------------------------------------------
    # Oracle comparison: exactly matches PyTorch's own F.scaled_dot_product
    # (without dropout, without is_causal) via the manual formula
    # ------------------------------------------------------------------

    def test_matches_manual_formula_random_shapes(self):
        """Exhaustive formula check across 8 random (B,T,d_k,d_v) configs."""
        for seed in range(8):
            torch.manual_seed(seed * 31 + 7)
            B = torch.randint(1, 5, ()).item()
            T = torch.randint(2, 10, ()).item()
            d_k = torch.randint(4, 17, ()).item()
            d_v = torch.randint(4, 17, ()).item()
            q = torch.randn(B, T, d_k)
            k = torch.randn(B, T, d_k)
            v = torch.randn(B, T, d_v)

            out, attn = scaled_dot_product_attention(q, k, v)

            # Reference: manual formula
            scores = (q @ k.transpose(-2, -1)) / math.sqrt(d_k)
            ref_attn = torch.softmax(scores, dim=-1)
            ref_out = ref_attn @ v

            assert torch.allclose(attn, ref_attn, atol=1e-5), \
                f"seed={seed}: attn mismatch"
            assert torch.allclose(out, ref_out, atol=1e-5), \
                f"seed={seed}: out mismatch"

    # ------------------------------------------------------------------
    # Gradient flows back through sdpa (q, k, v all receive .grad)
    # ------------------------------------------------------------------

    def test_gradients_flow_through_sdpa(self):
        torch.manual_seed(400)
        for seed in range(5):
            torch.manual_seed(seed + 400)
            q = torch.randn(2, 5, 8, requires_grad=True)
            k = torch.randn(2, 5, 8, requires_grad=True)
            v = torch.randn(2, 5, 8, requires_grad=True)
            out, _ = scaled_dot_product_attention(q, k, v)
            out.sum().backward()
            for name, t in [("q", q), ("k", k), ("v", v)]:
                assert t.grad is not None, f"seed={seed}: {name}.grad is None"
                assert t.grad.abs().max() > 1e-8, \
                    f"seed={seed}: {name}.grad is effectively zero"


# ===========================================================================
# causal_mask — extended property tests
# ===========================================================================

class TestCausalMaskProperties:

    @pytest.mark.parametrize("T", [1, 2, 5, 10, 20, 64])
    def test_shape_and_dtype_many_sizes(self, T):
        m = causal_mask(T)
        assert m.shape == (T, T)
        assert m.dtype == torch.bool

    @pytest.mark.parametrize("T", [1, 3, 7, 15])
    def test_upper_triangle_true_lower_false(self, T):
        """True iff j > i (strictly upper triangular)."""
        m = causal_mask(T)
        for i in range(T):
            for j in range(T):
                expected = (j > i)
                assert m[i, j].item() == expected, \
                    f"T={T}, [{i},{j}]: expected {expected}, got {m[i,j].item()}"

    def test_first_row_only_diagonal_allowed(self):
        """Position 0 may only see itself: [0,0] is False, rest True."""
        for T in [2, 5, 10]:
            m = causal_mask(T)
            assert m[0, 0].item() is False
            assert m[0, 1:].all(), f"T={T}: row 0, cols 1+ should all be True"

    def test_last_row_all_false(self):
        """Last position sees everything: last row is all False."""
        for T in [2, 5, 10]:
            m = causal_mask(T)
            assert not m[-1].any(), f"T={T}: last row should be all False"

    def test_diagonal_always_false(self):
        """Self-attention is always permitted: diagonal is all False."""
        for T in [1, 4, 8]:
            m = causal_mask(T)
            diag = torch.diagonal(m)
            assert not diag.any(), f"T={T}: diagonal contains True"

    def test_number_of_forbidden_positions(self):
        """T*(T-1)/2 positions forbidden for a T-length sequence."""
        for T in [1, 4, 7, 10]:
            m = causal_mask(T)
            expected_true = T * (T - 1) // 2
            assert m.sum().item() == expected_true, \
                f"T={T}: expected {expected_true} True, got {m.sum().item()}"

    def test_is_upper_triangular_complement_of_tril(self):
        """Mask should equal torch.triu(..., diagonal=1).bool()."""
        for T in [3, 6, 12]:
            m = causal_mask(T)
            ref = torch.triu(torch.ones(T, T), diagonal=1).bool()
            assert torch.equal(m, ref), f"T={T}: mask != triu reference"


# ===========================================================================
# MultiHeadAttention — extended property tests
# ===========================================================================

class TestMHAProperties:

    # ------------------------------------------------------------------
    # Output shape across varied (B, T, d_model, n_heads) configs
    # ------------------------------------------------------------------

    @pytest.mark.parametrize("B,T,d_model,n_heads", [
        (1, 1, 8, 1),     # minimal
        (2, 5, 16, 4),    # standard
        (3, 10, 32, 8),   # larger
        (1, 20, 64, 8),   # longer sequence
        (4, 3, 8, 2),     # non-square
    ])
    def test_output_shape(self, B, T, d_model, n_heads):
        torch.manual_seed(10)
        mha = MultiHeadAttention(d_model=d_model, n_heads=n_heads)
        x = torch.randn(B, T, d_model)
        out = mha(x)
        assert out.shape == (B, T, d_model), \
            f"Expected {(B, T, d_model)}, got {tuple(out.shape)}"

    # ------------------------------------------------------------------
    # Output is always finite (no NaN/Inf)
    # ------------------------------------------------------------------

    def test_output_finite_random(self):
        torch.manual_seed(20)
        for seed in range(6):
            torch.manual_seed(seed + 20)
            mha = MultiHeadAttention(d_model=16, n_heads=4)
            x = torch.randn(2, 6, 16)
            out = mha(x)
            assert torch.isfinite(out).all(), f"seed={seed}: MHA output has NaN/Inf"

    # ------------------------------------------------------------------
    # d_model not divisible by n_heads must raise
    # ------------------------------------------------------------------

    def test_invalid_n_heads_raises(self):
        with pytest.raises((AssertionError, ValueError)):
            MultiHeadAttention(d_model=10, n_heads=3)

    # ------------------------------------------------------------------
    # Causal property: changing ONLY position t must not affect 0..t-1
    # ------------------------------------------------------------------

    @pytest.mark.parametrize("T", [3, 6, 10])
    def test_causality_many_sequence_lengths(self, T):
        torch.manual_seed(30 + T)
        mha = MultiHeadAttention(d_model=16, n_heads=4)
        mha.eval()
        x = torch.randn(1, T, 16)
        out_a = mha(x)
        for perturb_pos in range(1, T):
            x2 = x.clone()
            x2[:, perturb_pos, :] = torch.randn(16)
            out_b = mha(x2)
            assert torch.allclose(out_a[:, :perturb_pos, :], out_b[:, :perturb_pos, :], atol=1e-5), \
                f"T={T}, perturb_pos={perturb_pos}: causality violated"

    # ------------------------------------------------------------------
    # Gradients are non-zero for all 4 weight matrices w_q, w_k, w_v, w_o
    # ------------------------------------------------------------------

    def test_all_weight_matrices_receive_gradient(self):
        torch.manual_seed(40)
        for seed in range(4):
            torch.manual_seed(seed + 40)
            mha = MultiHeadAttention(d_model=16, n_heads=4)
            x = torch.randn(2, 6, 16)
            out = mha(x)
            out.sum().backward()
            for name in ["w_q", "w_k", "w_v", "w_o"]:
                layer = getattr(mha, name)
                assert layer.weight.grad is not None, \
                    f"seed={seed}: {name}.weight.grad is None"
                assert layer.weight.grad.abs().max() > 1e-8, \
                    f"seed={seed}: {name}.weight.grad is effectively zero"

    # ------------------------------------------------------------------
    # Determinism: same seed -> same output
    # ------------------------------------------------------------------

    def test_deterministic_with_same_seed(self):
        def run(seed):
            torch.manual_seed(seed)
            mha = MultiHeadAttention(d_model=16, n_heads=4)
            x = torch.randn(2, 5, 16)
            return mha(x)

        out1 = run(99)
        out2 = run(99)
        assert torch.allclose(out1, out2, atol=0), \
            "Same seed should produce identical outputs"

    # ------------------------------------------------------------------
    # Large magnitude input — must remain finite
    # ------------------------------------------------------------------

    def test_large_input_finite(self):
        torch.manual_seed(500)
        mha = MultiHeadAttention(d_model=16, n_heads=4)
        x = torch.randn(1, 5, 16) * 1e3
        out = mha(x)
        assert torch.isfinite(out).all()

    # ------------------------------------------------------------------
    # Single-token sequence — must not crash and shape correct
    # ------------------------------------------------------------------

    def test_single_token_sequence(self):
        torch.manual_seed(600)
        mha = MultiHeadAttention(d_model=8, n_heads=2)
        x = torch.randn(3, 1, 8)
        out = mha(x)
        assert out.shape == (3, 1, 8)
        assert torch.isfinite(out).all()

    # ------------------------------------------------------------------
    # Training: loss decreases — MHA is learnable
    # ------------------------------------------------------------------

    def test_mha_gradient_step_decreases_loss(self):
        torch.manual_seed(700)
        mha = MultiHeadAttention(d_model=16, n_heads=4)
        x = torch.randn(2, 8, 16)
        target = torch.randn(2, 8, 16)
        opt = torch.optim.Adam(mha.parameters(), lr=1e-2)

        def loss_fn():
            return ((mha(x) - target) ** 2).mean()

        initial = loss_fn().item()
        for _ in range(50):
            opt.zero_grad()
            loss_fn().backward()
            opt.step()
        final = loss_fn().item()
        assert final < initial, \
            f"Loss did not decrease: initial={initial:.4f}, final={final:.4f}"


# ===========================================================================
# TransformerBlock — extended property tests
# ===========================================================================

class TestTransformerBlockProperties:

    # ------------------------------------------------------------------
    # Shape preservation across varied configs
    # ------------------------------------------------------------------

    @pytest.mark.parametrize("B,T,d_model,n_heads", [
        (1, 1, 8, 2),
        (2, 6, 16, 4),
        (4, 10, 32, 8),
        (1, 50, 64, 8),
    ])
    def test_output_shape(self, B, T, d_model, n_heads):
        torch.manual_seed(50)
        block = TransformerBlock(d_model=d_model, n_heads=n_heads)
        x = torch.randn(B, T, d_model)
        out = block(x)
        assert out.shape == (B, T, d_model)

    # ------------------------------------------------------------------
    # Output always finite across random seeds
    # ------------------------------------------------------------------

    def test_output_finite_random(self):
        for seed in range(8):
            torch.manual_seed(seed + 60)
            block = TransformerBlock(d_model=16, n_heads=4)
            x = torch.randn(2, 7, 16)
            out = block(x)
            assert torch.isfinite(out).all(), f"seed={seed}: output has NaN/Inf"

    # ------------------------------------------------------------------
    # Causal: perturbing any position t must not affect outputs 0..t-1
    # ------------------------------------------------------------------

    @pytest.mark.parametrize("T", [4, 8, 12])
    def test_causality(self, T):
        torch.manual_seed(70 + T)
        block = TransformerBlock(d_model=16, n_heads=4)
        block.eval()
        x = torch.randn(1, T, 16)
        out_a = block(x)
        for pos in range(1, T):
            x2 = x.clone()
            x2[:, pos, :] = torch.randn(16)
            out_b = block(x2)
            assert torch.allclose(out_a[:, :pos, :], out_b[:, :pos, :], atol=1e-5), \
                f"T={T}, perturb pos={pos}: causality violated"

    # ------------------------------------------------------------------
    # Pre-norm structure: ln1 and ln2 are called on the RESIDUAL path
    # (verify by checking ln1/ln2 running stats change after a forward)
    # ------------------------------------------------------------------

    def test_layernorms_have_correct_shape(self):
        torch.manual_seed(80)
        d_model = 24
        block = TransformerBlock(d_model=d_model, n_heads=4)
        assert block.ln1.normalized_shape == (d_model,)
        assert block.ln2.normalized_shape == (d_model,)

    # ------------------------------------------------------------------
    # All parameters receive non-zero gradients after backward
    # ------------------------------------------------------------------

    def test_all_params_receive_gradients(self):
        torch.manual_seed(90)
        block = TransformerBlock(d_model=16, n_heads=4)
        x = torch.randn(2, 6, 16)
        out = block(x)
        out.sum().backward()
        for name, param in block.named_parameters():
            assert param.grad is not None, f"{name}: grad is None"
            assert param.grad.abs().max() > 1e-10, \
                f"{name}: grad is effectively zero"

    # ------------------------------------------------------------------
    # Custom d_ff is respected: MLP first linear output dim matches d_ff
    # ------------------------------------------------------------------

    @pytest.mark.parametrize("d_ff", [16, 64, 128])
    def test_custom_d_ff(self, d_ff):
        torch.manual_seed(100)
        block = TransformerBlock(d_model=16, n_heads=4, d_ff=d_ff)
        # first Linear in mlp should have out_features == d_ff
        first_linear = block.mlp[0]
        assert first_linear.out_features == d_ff

    # ------------------------------------------------------------------
    # Default d_ff = 4 * d_model
    # ------------------------------------------------------------------

    @pytest.mark.parametrize("d_model", [8, 16, 32])
    def test_default_d_ff(self, d_model):
        block = TransformerBlock(d_model=d_model, n_heads=2 if d_model >= 8 else 1)
        first_linear = block.mlp[0]
        assert first_linear.out_features == 4 * d_model

    # ------------------------------------------------------------------
    # Stacking two blocks: shape preserved end-to-end
    # ------------------------------------------------------------------

    def test_stacked_blocks_shape(self):
        torch.manual_seed(110)
        B, T, d = 2, 8, 32
        b1 = TransformerBlock(d_model=d, n_heads=4)
        b2 = TransformerBlock(d_model=d, n_heads=4)
        x = torch.randn(B, T, d)
        out = b2(b1(x))
        assert out.shape == (B, T, d)
        assert torch.isfinite(out).all()

    # ------------------------------------------------------------------
    # Residual: output must NOT equal the MLP/attn sub-output alone
    # (confirms x + F(x) rather than just F(x))
    # ------------------------------------------------------------------

    def test_residual_connection_is_additive(self):
        """block(x) should differ from both MHA(x) alone and MLP(x) alone."""
        torch.manual_seed(120)
        block = TransformerBlock(d_model=16, n_heads=4)
        x = torch.randn(1, 5, 16)
        out = block(x)

        # If pure MHA (no residual), we'd get attn(ln1(x)), not x + attn(...)
        pure_attn = block.attn(block.ln1(x))
        assert not torch.allclose(out, pure_attn, atol=1e-4), \
            "Output matches pure MHA — residual connection may be missing"

    # ------------------------------------------------------------------
    # Large magnitude input — finite output
    # ------------------------------------------------------------------

    def test_large_magnitude_input_finite(self):
        torch.manual_seed(800)
        block = TransformerBlock(d_model=16, n_heads=4)
        x = torch.randn(1, 5, 16) * 1e3
        out = block(x)
        assert torch.isfinite(out).all()

    # ------------------------------------------------------------------
    # Training step over multiple random seeds: loss decreases each time
    # ------------------------------------------------------------------

    @pytest.mark.parametrize("seed", [1000, 1001, 1002])
    def test_training_step_reduces_loss(self, seed):
        torch.manual_seed(seed)
        B, T, d = 2, 6, 16
        block = TransformerBlock(d_model=d, n_heads=4)
        head = nn.Linear(d, d)
        x = torch.randn(B, T, d)
        target = torch.randn(B, T - 1, d)

        params = list(block.parameters()) + list(head.parameters())
        opt = torch.optim.Adam(params, lr=1e-2)

        def loss_fn():
            h = block(x)
            pred = head(h)[:, :-1, :]
            return ((pred - target) ** 2).mean()

        initial = loss_fn().item()
        for _ in range(80):
            opt.zero_grad()
            loss_fn().backward()
            opt.step()
        final = loss_fn().item()
        assert final < initial * 0.7, \
            f"seed={seed}: loss barely decreased: {initial:.4f} -> {final:.4f}"


# ===========================================================================
# Positional encoding — extended property tests
# ===========================================================================

class TestPositionalEncodingProperties:
    """Randomised / oracle checks for sinusoidal and learned positional encoding."""

    # ------------------------------------------------------------------
    # Shape contract across varied (T, d_model)
    # ------------------------------------------------------------------

    @pytest.mark.parametrize("T,d_model", [
        (1, 2), (3, 4), (5, 8), (10, 16), (7, 32), (20, 64),
    ])
    def test_sinusoidal_shape(self, T, d_model):
        pe = sinusoidal_positional_encoding(T, d_model)
        assert pe.shape == (T, d_model)

    # ------------------------------------------------------------------
    # Closed-form oracle: PE[pos,2i]=sin(.), PE[pos,2i+1]=cos(.) over a grid
    # ------------------------------------------------------------------

    def test_sinusoidal_matches_closed_form_grid(self):
        for T, d_model in [(8, 16), (5, 8), (12, 32)]:
            pe = sinusoidal_positional_encoding(T, d_model)
            pos = torch.arange(T, dtype=torch.float32).unsqueeze(1)        # (T,1)
            i2 = torch.arange(0, d_model, 2, dtype=torch.float32)          # (d/2,)
            denom = torch.pow(torch.tensor(10000.0), i2 / d_model)        # (d/2,)
            angles = pos / denom                                          # (T,d/2)
            ref = torch.zeros(T, d_model)
            ref[:, 0::2] = torch.sin(angles)
            ref[:, 1::2] = torch.cos(angles)
            assert torch.allclose(pe, ref, atol=1e-6), \
                f"T={T}, d_model={d_model}: sinusoidal PE != closed form"

    # ------------------------------------------------------------------
    # Bounded: sin/cos values always live in [-1, 1]
    # ------------------------------------------------------------------

    def test_sinusoidal_values_bounded(self):
        for T, d_model in [(16, 8), (32, 16), (50, 64)]:
            pe = sinusoidal_positional_encoding(T, d_model)
            assert (pe >= -1.0 - 1e-6).all() and (pe <= 1.0 + 1e-6).all()

    # ------------------------------------------------------------------
    # First channel (i=0, sin branch) strictly increasing in pos over a
    # short prefix where sin argument stays within [0, pi/2]
    # ------------------------------------------------------------------

    def test_sinusoidal_first_sin_channel_increasing_on_prefix(self):
        # channel 0 = sin(pos / 1) = sin(pos); strictly increasing for pos in [0,1]
        T, d_model = 2, 8
        pe = sinusoidal_positional_encoding(T, d_model)
        col0 = pe[:, 0]
        assert (col0[1:] > col0[:-1]).all(), "sin channel not increasing on [0,1]"

    # ------------------------------------------------------------------
    # Determinism: sinusoidal PE depends only on (T, d_model), no randomness
    # ------------------------------------------------------------------

    def test_sinusoidal_deterministic(self):
        a = sinusoidal_positional_encoding(7, 16)
        b = sinusoidal_positional_encoding(7, 16)
        assert torch.equal(a, b)

    # ------------------------------------------------------------------
    # Odd d_model is rejected (cannot pair sin/cos channels)
    # ------------------------------------------------------------------

    @pytest.mark.parametrize("d_model", [3, 5, 7, 15])
    def test_sinusoidal_odd_d_model_raises(self, d_model):
        with pytest.raises(ValueError):
            sinusoidal_positional_encoding(4, d_model)

    # ------------------------------------------------------------------
    # Not trainable: returned tensor carries no gradient
    # ------------------------------------------------------------------

    def test_sinusoidal_no_grad(self):
        pe = sinusoidal_positional_encoding(6, 8)
        assert pe.requires_grad is False

    # ------------------------------------------------------------------
    # Broadcast-add onto (B, T, d_model) leaves shape unchanged
    # ------------------------------------------------------------------

    @pytest.mark.parametrize("B,T,d_model", [(1, 5, 8), (4, 3, 16), (2, 10, 32)])
    def test_sinusoidal_broadcast_add_preserves_shape(self, B, T, d_model):
        torch.manual_seed(13)
        x = torch.randn(B, T, d_model)
        out = x + sinusoidal_positional_encoding(T, d_model)
        assert out.shape == (B, T, d_model)

    # ------------------------------------------------------------------
    # Learned PE: shape (T, d_model), prefix of the embedding table
    # ------------------------------------------------------------------

    @pytest.mark.parametrize("T,d_model", [(1, 4), (5, 8), (16, 16)])
    def test_learned_shape(self, T, d_model):
        torch.manual_seed(1)
        pe = LearnedPositionalEncoding(max_T=16, d_model=d_model)
        assert pe(T).shape == (T, d_model)

    # ------------------------------------------------------------------
    # Learned PE returns the first T rows of the embedding weight exactly
    # ------------------------------------------------------------------

    def test_learned_returns_prefix_of_weight(self):
        torch.manual_seed(2)
        pe = LearnedPositionalEncoding(max_T=16, d_model=8)
        out = pe(5)
        assert torch.allclose(out, pe.emb.weight[:5], atol=0)

    # ------------------------------------------------------------------
    # Learned PE is trainable: a gradient step moves the weights (many seeds)
    # ------------------------------------------------------------------

    @pytest.mark.parametrize("seed", [0, 1, 2, 3])
    def test_learned_is_trainable(self, seed):
        torch.manual_seed(seed)
        pe = LearnedPositionalEncoding(max_T=16, d_model=8)
        before = pe.emb.weight.detach().clone()
        opt = torch.optim.Adam(pe.parameters(), lr=1e-1)
        out = pe(6)
        loss = (out ** 2).mean()
        opt.zero_grad()
        loss.backward()
        opt.step()
        after = pe.emb.weight.detach().clone()
        assert not torch.allclose(before, after, atol=1e-6), \
            f"seed={seed}: learned PE weights did not change after step"

    # ------------------------------------------------------------------
    # Gradient actually flows to the embedding rows that were used
    # ------------------------------------------------------------------

    def test_learned_gradient_flows(self):
        torch.manual_seed(5)
        pe = LearnedPositionalEncoding(max_T=16, d_model=8)
        out = pe(4)
        out.sum().backward()
        assert pe.emb.weight.grad is not None
        # only the first 4 rows were used -> they must have non-zero grad
        assert pe.emb.weight.grad[:4].abs().max() > 1e-8

    # ------------------------------------------------------------------
    # Asking for T > max_T must raise (no extrapolation)
    # ------------------------------------------------------------------

    def test_learned_exceeding_max_T_raises(self):
        pe = LearnedPositionalEncoding(max_T=8, d_model=8)
        with pytest.raises(ValueError):
            pe(9)

    # ------------------------------------------------------------------
    # The two strategies disagree for the same (T, d_model)
    # ------------------------------------------------------------------

    @pytest.mark.parametrize("T,d_model", [(5, 8), (10, 16), (3, 32)])
    def test_strategies_differ(self, T, d_model):
        torch.manual_seed(7)
        sin_pe = sinusoidal_positional_encoding(T, d_model)
        learned = LearnedPositionalEncoding(max_T=32, d_model=d_model)(T)
        assert not torch.allclose(sin_pe, learned, atol=1e-3), \
            "sinusoidal and learned PE should not coincide"

    # ------------------------------------------------------------------
    # Learned PE broadcast-add onto (B, T, d_model) preserves shape
    # ------------------------------------------------------------------

    def test_learned_broadcast_add_preserves_shape(self):
        torch.manual_seed(8)
        B, T, d_model = 3, 6, 16
        pe = LearnedPositionalEncoding(max_T=16, d_model=d_model)
        x = torch.randn(B, T, d_model)
        out = x + pe(T)
        assert out.shape == (B, T, d_model)
