# Randomized / property-based tests for Module 14 — Sequence Models.
#
# Rules:
#   - Every test is self-contained and deterministic (fixed seed at the top).
#   - All imports of the solution happen INSIDE test functions, so the file
#     collects cleanly against an unfilled stub (raises NotImplementedError
#     only at runtime, not at import / collection time).
#   - Do NOT duplicate the fixed cases already in test_seqmodels.py.

import numpy as np  # noqa: F401
import pytest
import torch
import torch.nn as nn


# ---------------------------------------------------------------------------
# Helpers (no solution calls at module level)
# ---------------------------------------------------------------------------

def _make_model(vocab_size, embed_dim, hidden_size, seed=42):
    from seqmodels import CharRNN  # noqa: PLC0415
    torch.manual_seed(seed)
    return CharRNN(vocab_size, embed_dim, hidden_size)


def _rand_batch(vocab_size, B, T, seed):
    g = torch.Generator().manual_seed(seed)
    X = torch.randint(0, vocab_size, (B, T), generator=g)
    Y = torch.roll(X, shifts=-1, dims=1)
    return X, Y


# ---------------------------------------------------------------------------
# Shape invariants across many random (B, T, vocab, embed, hidden) configs
# ---------------------------------------------------------------------------

@pytest.mark.parametrize("seed,vocab_size,embed_dim,hidden_size,B,T", [
    (10, 3,  2,  4,  1,  1),   # minimal — single token, single example
    (11, 5,  4,  6,  2,  3),
    (12, 10, 8,  12, 4,  7),
    (13, 20, 16, 32, 8,  15),
    (14, 50, 32, 64, 16, 20),
])
def test_output_shape_parametric(seed, vocab_size, embed_dim, hidden_size, B, T):
    """Output shape must be exactly (B, T, vocab_size) across a grid of configs."""
    from seqmodels import CharRNN  # noqa: PLC0415
    torch.manual_seed(seed)
    model = CharRNN(vocab_size, embed_dim, hidden_size)
    x = torch.randint(0, vocab_size, (B, T))
    logits = model(x)
    assert tuple(logits.shape) == (B, T, vocab_size), (
        f"Expected ({B}, {T}, {vocab_size}), got {tuple(logits.shape)}"
    )


# ---------------------------------------------------------------------------
# dtype: output is always float32 regardless of integer input
# ---------------------------------------------------------------------------

def test_output_dtype_is_float32_for_various_inputs():
    """float32 output no matter which integer subtype or size the input has."""
    from seqmodels import CharRNN  # noqa: PLC0415
    torch.manual_seed(20)
    model = CharRNN(vocab_size=8, embed_dim=4, hidden_size=8)
    model.eval()
    for B, T, seed in [(1, 1, 0), (2, 10, 1), (5, 5, 2)]:
        x = torch.randint(0, 8, (B, T)).long()  # explicitly long
        logits = model(x)
        assert logits.dtype == torch.float32, f"dtype was {logits.dtype}"


# ---------------------------------------------------------------------------
# Determinism: same input + same weights → same output (eval mode)
# ---------------------------------------------------------------------------

def test_forward_is_deterministic():
    """Two forward passes with the same input produce identical results."""
    from seqmodels import CharRNN  # noqa: PLC0415
    torch.manual_seed(30)
    model = CharRNN(vocab_size=7, embed_dim=5, hidden_size=9)
    model.eval()
    x = torch.randint(0, 7, (3, 12))
    out1 = model(x)
    out2 = model(x)
    assert torch.allclose(out1, out2, atol=0.0), "forward is not deterministic"


# ---------------------------------------------------------------------------
# Batch independence: one row's logits unchanged when other rows differ
# ---------------------------------------------------------------------------

def test_batch_independence_different_rows():
    """Changing one row in a batch must NOT affect other rows' logits."""
    from seqmodels import CharRNN  # noqa: PLC0415
    torch.manual_seed(40)
    vocab_size = 9
    model = CharRNN(vocab_size, embed_dim=6, hidden_size=10)
    model.eval()

    g = torch.Generator().manual_seed(40)
    row0 = torch.randint(0, vocab_size, (1, 8), generator=g)
    row1_a = torch.randint(0, vocab_size, (1, 8), generator=g)
    row1_b = torch.randint(0, vocab_size, (1, 8), generator=g)  # different

    out_a = model(torch.cat([row0, row1_a], dim=0))
    out_b = model(torch.cat([row0, row1_b], dim=0))

    # row 0 logits must be identical regardless of what row 1 is
    assert torch.allclose(out_a[0], out_b[0], atol=1e-6), (
        "row 0 logits changed when row 1 changed — batch independence violated"
    )


# ---------------------------------------------------------------------------
# Embedding layer: lookup matches nn.Embedding directly
# ---------------------------------------------------------------------------

def test_embedding_lookup_matches_torch_embedding():
    """The embedding sub-layer performs an exact table lookup — verify against
    a manual extraction of the same weight matrix."""
    from seqmodels import CharRNN  # noqa: PLC0415
    torch.manual_seed(50)
    vocab_size, embed_dim, hidden_size = 6, 4, 8
    model = CharRNN(vocab_size, embed_dim, hidden_size)
    model.eval()

    x = torch.tensor([[2, 0, 5, 1, 3]])  # (1, 5) known indices
    with torch.no_grad():
        # Extract embedding weight and look up manually
        W = model.embed.weight  # (vocab_size, embed_dim)
        expected_emb = W[x[0]]  # (5, embed_dim)
        actual_emb = model.embed(x)  # (1, 5, embed_dim)
    assert torch.allclose(actual_emb[0], expected_emb, atol=1e-7), (
        "Embedding output does not match direct weight-row lookup"
    )


# ---------------------------------------------------------------------------
# RNN outputs: hidden state shape and batch_first=True contract
# ---------------------------------------------------------------------------

def test_rnn_hidden_state_shape():
    """nn.RNN with batch_first=True must produce h_n of shape (1, B, hidden_size)."""
    from seqmodels import CharRNN  # noqa: PLC0415
    torch.manual_seed(60)
    B, T = 4, 7
    vocab_size, embed_dim, hidden_size = 10, 6, 12
    model = CharRNN(vocab_size, embed_dim, hidden_size)
    model.eval()

    x = torch.randint(0, vocab_size, (B, T))
    with torch.no_grad():
        emb = model.embed(x)                  # (B, T, embed_dim)
        out, h_n = model.rnn(emb)             # out: (B, T, hidden_size)

    assert tuple(out.shape) == (B, T, hidden_size), f"out shape: {out.shape}"
    assert tuple(h_n.shape) == (1, B, hidden_size), f"h_n shape: {h_n.shape}"


def test_last_rnn_output_equals_h_n():
    """The last time-step of `out` must equal h_n (squeezed)."""
    from seqmodels import CharRNN  # noqa: PLC0415
    torch.manual_seed(61)
    B, T = 3, 5
    vocab_size, embed_dim, hidden_size = 8, 4, 10
    model = CharRNN(vocab_size, embed_dim, hidden_size)
    model.eval()

    x = torch.randint(0, vocab_size, (B, T))
    with torch.no_grad():
        emb = model.embed(x)
        out, h_n = model.rnn(emb)

    # out[:, -1, :] should equal h_n[0]
    assert torch.allclose(out[:, -1, :], h_n[0], atol=1e-6), (
        "Last RNN output does not match h_n — batch_first contract broken"
    )


# ---------------------------------------------------------------------------
# Linear layer: fc maps hidden_size -> vocab_size correctly
# ---------------------------------------------------------------------------

def test_fc_layer_matches_direct_linear():
    """The fc sub-layer applied to RNN output matches a manual matmul+bias."""
    from seqmodels import CharRNN  # noqa: PLC0415
    torch.manual_seed(70)
    vocab_size, embed_dim, hidden_size = 7, 5, 8
    B, T = 2, 6
    model = CharRNN(vocab_size, embed_dim, hidden_size)
    model.eval()

    x = torch.randint(0, vocab_size, (B, T))
    with torch.no_grad():
        emb = model.embed(x)
        out, _ = model.rnn(emb)          # (B, T, hidden_size)
        logits_model = model.fc(out)     # (B, T, vocab_size)

        W = model.fc.weight              # (vocab_size, hidden_size)
        b = model.fc.bias                # (vocab_size,)
        logits_manual = out @ W.T + b    # (B, T, vocab_size)

    assert torch.allclose(logits_model, logits_manual, atol=1e-5), (
        "fc output does not match manual W·h + b"
    )


# ---------------------------------------------------------------------------
# Gradients: every parameter gets a nonzero gradient after one backward pass
# ---------------------------------------------------------------------------

def test_all_parameters_receive_nonzero_gradient():
    """After one backward pass, every parameter has .grad not None and nonzero."""
    from seqmodels import CharRNN  # noqa: PLC0415
    torch.manual_seed(80)
    vocab_size, embed_dim, hidden_size = 8, 6, 10
    B, T = 3, 8
    model = CharRNN(vocab_size, embed_dim, hidden_size)

    x = torch.randint(0, vocab_size, (B, T))
    logits = model(x)                                         # (B, T, vocab_size)
    loss = logits.sum()                                       # scalar, touches all outputs
    loss.backward()

    for name, p in model.named_parameters():
        assert p.grad is not None, f"param '{name}' has no gradient"
        assert p.grad.abs().sum().item() > 0.0, (
            f"param '{name}' has zero gradient — not connected to output"
        )


def test_gradient_flows_through_embedding():
    """Embedding rows corresponding to input indices must receive gradients."""
    from seqmodels import CharRNN  # noqa: PLC0415
    torch.manual_seed(81)
    vocab_size, embed_dim, hidden_size = 10, 4, 8
    B, T = 2, 5
    model = CharRNN(vocab_size, embed_dim, hidden_size)

    # Use only indices 0..4 — only those rows should get gradients
    x = torch.randint(0, 5, (B, T))
    logits = model(x)
    logits.sum().backward()

    W_grad = model.embed.weight.grad  # (vocab_size, embed_dim)
    assert W_grad is not None, "embed.weight has no gradient"
    # Used rows (0-4) should have nonzero grad
    assert W_grad[:5].abs().sum().item() > 0.0, "used embedding rows have zero grad"


# ---------------------------------------------------------------------------
# train_step: gradients zeroed before each step (no accumulation)
# ---------------------------------------------------------------------------

def test_train_step_zeros_gradients_before_step():
    """Calling train_step twice must not accumulate gradients from the first call."""
    from seqmodels import CharRNN, train_step  # noqa: PLC0415
    torch.manual_seed(90)
    vocab_size = 6
    model = CharRNN(vocab_size, embed_dim=4, hidden_size=8)
    optimizer = torch.optim.SGD(model.parameters(), lr=0.0)  # no weight update
    loss_fn = nn.CrossEntropyLoss()

    X, Y = _rand_batch(vocab_size, B=2, T=5, seed=90)

    # Two steps with lr=0 (weights don't change) — gradient after step 2
    # should equal gradient from step 2 alone, not the sum of both
    train_step(model, optimizer, loss_fn, X, Y)
    grad_after_step1 = {
        n: p.grad.clone() for n, p in model.named_parameters() if p.grad is not None
    }
    train_step(model, optimizer, loss_fn, X, Y)
    grad_after_step2 = {
        n: p.grad.clone() for n, p in model.named_parameters() if p.grad is not None
    }

    # Because the inputs and weights are identical across both steps, the
    # gradients should be equal (not doubled by accumulation)
    for name in grad_after_step1:
        assert torch.allclose(grad_after_step1[name], grad_after_step2[name], atol=1e-6), (
            f"Gradient for '{name}' looks accumulated across steps (not zeroed)"
        )


# ---------------------------------------------------------------------------
# train_step: loss decreases over many steps on a memorizable pattern
# ---------------------------------------------------------------------------

@pytest.mark.parametrize("seed,vocab_size,B,T,steps", [
    (100, 4,  1, 10, 300),
    (101, 6,  2, 8,  250),
    (102, 8,  3, 12, 300),
])
def test_training_decreases_loss_parametric(seed, vocab_size, B, T, steps):
    """Loss must drop substantially (< 0.5× initial) for multiple configs."""
    from seqmodels import CharRNN, train_step  # noqa: PLC0415
    torch.manual_seed(seed)
    model = CharRNN(vocab_size, embed_dim=8, hidden_size=16)
    optimizer = torch.optim.Adam(model.parameters(), lr=0.01)
    loss_fn = nn.CrossEntropyLoss()

    X, Y = _rand_batch(vocab_size, B, T, seed=seed)

    initial = train_step(model, optimizer, loss_fn, X, Y)
    final = initial
    for _ in range(steps - 1):
        final = train_step(model, optimizer, loss_fn, X, Y)

    assert final < 0.5 * initial, (
        f"seed={seed}: loss did not drop enough ({initial:.4f} -> {final:.4f})"
    )


# ---------------------------------------------------------------------------
# train_step: returned loss matches cross-entropy on the PRE-STEP model state
# ---------------------------------------------------------------------------

def test_train_step_returns_pre_step_loss_multiple_seeds():
    """For several random inits, returned loss == manual pre-step cross-entropy."""
    from seqmodels import CharRNN, train_step  # noqa: PLC0415

    for seed in [200, 201, 202, 203]:
        torch.manual_seed(seed)
        vocab_size = 7
        model = CharRNN(vocab_size, embed_dim=5, hidden_size=9)
        # lr=0 so no parameter moves
        optimizer = torch.optim.SGD(model.parameters(), lr=0.0)
        loss_fn = nn.CrossEntropyLoss()
        X, Y = _rand_batch(vocab_size, B=3, T=6, seed=seed)

        with torch.no_grad():
            logits = model(X)
            expected = loss_fn(logits.reshape(-1, vocab_size), Y.reshape(-1)).item()

        got = train_step(model, optimizer, loss_fn, X, Y)
        assert abs(got - expected) < 1e-5, (
            f"seed={seed}: returned {got:.6f}, expected {expected:.6f}"
        )


# ---------------------------------------------------------------------------
# Numerical stability: large-magnitude inputs do not produce NaN / Inf
# ---------------------------------------------------------------------------

def test_forward_no_nan_on_various_random_inputs():
    """logits must be finite for normal random integer indices across many seeds."""
    from seqmodels import CharRNN  # noqa: PLC0415
    torch.manual_seed(300)
    model = CharRNN(vocab_size=15, embed_dim=10, hidden_size=20)
    model.eval()
    for seed in range(10):
        x = torch.randint(0, 15, (4, 16))
        logits = model(x)
        assert torch.isfinite(logits).all(), (
            f"seed {seed}: NaN or Inf in logits — numerical stability broken"
        )


def test_forward_no_nan_after_many_training_steps():
    """Weights should stay finite through extended training."""
    from seqmodels import CharRNN, train_step  # noqa: PLC0415
    torch.manual_seed(310)
    vocab_size = 10
    model = CharRNN(vocab_size, embed_dim=8, hidden_size=16)
    optimizer = torch.optim.Adam(model.parameters(), lr=0.01)
    loss_fn = nn.CrossEntropyLoss()

    X, Y = _rand_batch(vocab_size, B=4, T=10, seed=310)
    for _ in range(500):
        train_step(model, optimizer, loss_fn, X, Y)

    x_test = torch.randint(0, vocab_size, (4, 10))
    logits = model(x_test)
    assert torch.isfinite(logits).all(), "NaN/Inf in logits after extended training"


# ---------------------------------------------------------------------------
# Edge cases: B=1 (single example), T=1 (single time step)
# ---------------------------------------------------------------------------

def test_single_example_single_step():
    """B=1, T=1 is the smallest possible batch — must not crash or give wrong shape."""
    from seqmodels import CharRNN  # noqa: PLC0415
    torch.manual_seed(400)
    vocab_size, embed_dim, hidden_size = 5, 3, 7
    model = CharRNN(vocab_size, embed_dim, hidden_size)
    model.eval()

    x = torch.randint(0, vocab_size, (1, 1))
    logits = model(x)
    assert tuple(logits.shape) == (1, 1, vocab_size)
    assert torch.isfinite(logits).all()


def test_single_token_vocabulary():
    """vocab_size=1 is degenerate but must not crash and return correct shape."""
    from seqmodels import CharRNN  # noqa: PLC0415
    torch.manual_seed(401)
    model = CharRNN(vocab_size=1, embed_dim=2, hidden_size=4)
    model.eval()

    x = torch.zeros(2, 5, dtype=torch.long)  # all index 0
    logits = model(x)
    assert tuple(logits.shape) == (2, 5, 1)
    assert torch.isfinite(logits).all()


def test_all_same_token_batch():
    """All-zero input tensor (repeated index) must produce finite, consistent output."""
    from seqmodels import CharRNN  # noqa: PLC0415
    torch.manual_seed(402)
    vocab_size, embed_dim, hidden_size = 8, 4, 6
    B, T = 3, 10
    model = CharRNN(vocab_size, embed_dim, hidden_size)
    model.eval()

    x = torch.zeros(B, T, dtype=torch.long)  # all index 0
    logits = model(x)
    assert tuple(logits.shape) == (B, T, vocab_size)
    assert torch.isfinite(logits).all()
    # All rows of the same all-zero input must be identical
    for i in range(1, B):
        assert torch.allclose(logits[0], logits[i], atol=1e-6), (
            f"Row {i} differs from row 0 for identical all-zero input"
        )


# ---------------------------------------------------------------------------
# Module parameter count: embedding + rnn + linear — rough sanity check
# ---------------------------------------------------------------------------

def test_parameter_count_is_correct():
    """Total parameter count must match the sum of the three sub-layers."""
    from seqmodels import CharRNN  # noqa: PLC0415
    torch.manual_seed(500)
    vocab_size, embed_dim, hidden_size = 10, 6, 8
    model = CharRNN(vocab_size, embed_dim, hidden_size)

    # Expected:
    #   embed: vocab_size * embed_dim
    #   rnn:   (embed_dim + hidden_size + 1) * hidden_size  * 2 (weight_ih + weight_hh + bias_ih + bias_hh)
    #          = input_size * hidden_size + hidden_size * hidden_size + hidden_size + hidden_size
    #   linear: hidden_size * vocab_size + vocab_size
    embed_params  = vocab_size * embed_dim
    rnn_ih_params = embed_dim  * hidden_size
    rnn_hh_params = hidden_size * hidden_size
    rnn_b_params  = hidden_size + hidden_size   # bias_ih + bias_hh
    rnn_params    = rnn_ih_params + rnn_hh_params + rnn_b_params
    fc_params     = hidden_size * vocab_size + vocab_size

    expected_total = embed_params + rnn_params + fc_params
    actual_total   = sum(p.numel() for p in model.parameters())

    assert actual_total == expected_total, (
        f"Parameter count mismatch: expected {expected_total}, got {actual_total}"
    )


# ---------------------------------------------------------------------------
# RNN hidden state is initialised to zero (h_0 = 0 by default in PyTorch)
# ---------------------------------------------------------------------------

def test_rnn_initial_hidden_state_is_zero():
    """When no h_0 is passed, the RNN starts with all-zero hidden state."""
    from seqmodels import CharRNN  # noqa: PLC0415
    torch.manual_seed(510)
    vocab_size, embed_dim, hidden_size = 6, 4, 8
    B, T = 2, 5
    model = CharRNN(vocab_size, embed_dim, hidden_size)
    model.eval()

    x = torch.randint(0, vocab_size, (B, T))
    with torch.no_grad():
        emb = model.embed(x)
        # Pass explicit zeros for h_0
        h0 = torch.zeros(1, B, hidden_size)
        out_explicit, _ = model.rnn(emb, h0)
        # Default (no h_0)
        out_default, _ = model.rnn(emb)

    assert torch.allclose(out_explicit, out_default, atol=1e-7), (
        "RNN output differs when h_0=zeros is explicit vs default — "
        "default h_0 is not zero"
    )


# ---------------------------------------------------------------------------
# Cross-entropy loss shape: train_step correctly reshapes logits / targets
# ---------------------------------------------------------------------------

def test_train_step_reshape_is_correct():
    """The reshape (B,T,V)->(B*T,V) and (B,T)->(B*T) must give consistent loss."""
    from seqmodels import CharRNN, train_step  # noqa: PLC0415
    torch.manual_seed(520)
    vocab_size = 8
    B, T = 3, 7

    model = CharRNN(vocab_size, embed_dim=5, hidden_size=10)
    loss_fn = nn.CrossEntropyLoss()
    # lr=0 so weights don't move
    optimizer = torch.optim.SGD(model.parameters(), lr=0.0)

    X, Y = _rand_batch(vocab_size, B, T, seed=520)

    returned_loss = train_step(model, optimizer, loss_fn, X, Y)

    # Manually compute: same model, same inputs (weights unchanged because lr=0)
    with torch.no_grad():
        logits = model(X)                                              # (B, T, V)
        manual_loss = loss_fn(logits.reshape(-1, vocab_size), Y.reshape(-1)).item()

    assert abs(returned_loss - manual_loss) < 1e-5, (
        f"train_step loss {returned_loss:.6f} != manual {manual_loss:.6f} — "
        "reshape in train_step may be wrong"
    )


# ---------------------------------------------------------------------------
# Embedding-weight gradient is sparse: only accessed rows get nonzero grad
# ---------------------------------------------------------------------------

def test_embedding_gradient_sparse_unused_rows_zero():
    """Rows of the embedding matrix that are NOT in the input must have zero grad."""
    from seqmodels import CharRNN  # noqa: PLC0415
    torch.manual_seed(600)
    vocab_size = 20
    # Use only indices 0..4
    used_indices = list(range(5))
    model = CharRNN(vocab_size, embed_dim=6, hidden_size=12)

    x = torch.tensor(used_indices).unsqueeze(0)  # (1, 5)
    logits = model(x)
    logits.sum().backward()

    W_grad = model.embed.weight.grad  # (vocab_size, embed_dim)
    assert W_grad is not None
    # Rows 5..19 were not accessed — their gradients must be zero
    unused_grad_norm = W_grad[5:].abs().sum().item()
    assert unused_grad_norm == 0.0, (
        f"Unused embedding rows have nonzero gradient ({unused_grad_norm:.6f})"
    )


# ---------------------------------------------------------------------------
# forward output logits are NOT probabilities (no implicit softmax)
# ---------------------------------------------------------------------------

def test_logits_are_not_softmaxed():
    """CharRNN returns raw logits; their exp() should NOT sum to 1 per step."""
    from seqmodels import CharRNN  # noqa: PLC0415
    torch.manual_seed(700)
    vocab_size, embed_dim, hidden_size = 8, 4, 8
    B, T = 2, 6
    model = CharRNN(vocab_size, embed_dim, hidden_size)
    model.eval()

    x = torch.randint(0, vocab_size, (B, T))
    with torch.no_grad():
        logits = model(x)  # (B, T, vocab_size)

    # If logits were softmaxed, each row would sum to ~1
    row_sums = logits.exp().sum(dim=-1)  # (B, T) — sums of exp(logits)
    # For raw logits the row sum of exp is generally != 1
    # We check at least one row deviates from 1 by more than 0.01
    assert not torch.allclose(row_sums, torch.ones_like(row_sums), atol=0.01), (
        "logits appear to be softmaxed — forward should return raw logits"
    )


# ---------------------------------------------------------------------------
# train_step: loss is a positive finite float
# ---------------------------------------------------------------------------

@pytest.mark.parametrize("seed", [800, 801, 802, 803, 804])
def test_train_step_loss_is_positive_and_finite(seed):
    """Loss returned by train_step must be > 0 and finite for various seeds."""
    from seqmodels import CharRNN, train_step  # noqa: PLC0415
    torch.manual_seed(seed)
    vocab_size = 9
    model = CharRNN(vocab_size, embed_dim=6, hidden_size=12)
    optimizer = torch.optim.Adam(model.parameters(), lr=0.01)
    loss_fn = nn.CrossEntropyLoss()

    X, Y = _rand_batch(vocab_size, B=4, T=10, seed=seed)
    loss = train_step(model, optimizer, loss_fn, X, Y)

    assert isinstance(loss, float), f"Expected float, got {type(loss)}"
    assert loss > 0.0, f"Loss must be positive, got {loss}"
    assert np.isfinite(loss), f"Loss is not finite: {loss}"
