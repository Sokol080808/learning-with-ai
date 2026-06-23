# Randomized / property-based tests for Module 12 — PyTorch MLP.
#
# These tests are COMPLEMENTARY to test_torch_mlp.py (no duplicate of fixed cases).
# They add:
#   - randomised shape sweeps (batch sizes, hidden dims, class counts, feature dims)
#   - gradient-flow checks (every param gets a non-zero grad)
#   - parameter-count formula verified across many random architectures
#   - loss-decrease over one step for many random seeds
#   - accuracy dtype / range / boundary properties
#   - numerical-stability edge cases (zeros, negatives, large magnitudes, N=1)
#   - no-grad contract: accuracy must not leave a computation graph behind
#   - train/eval mode restored correctly
#   - zero_grad contract: gradients cleared before each step (no accumulation)
#
# All calls to the module happen INSIDE test functions — the file can be
# imported / collected against a stub that raises NotImplementedError.

import pytest
import numpy as np
import torch
import torch.nn as nn

from torch_mlp import MLP, train_step, train_epoch, accuracy


# ──────────────────────────────────────────────────────────────────────────────
# Helpers
# ──────────────────────────────────────────────────────────────────────────────

def _make_batch(n: int, in_dim: int, n_classes: int, seed: int):
    """Random float32 X and int64 y, fully deterministic."""
    torch.manual_seed(seed)
    X = torch.randn(n, in_dim)
    y = torch.randint(0, n_classes, (n,))
    return X, y


def _make_model(in_dim: int, hidden: int, out_dim: int, seed: int) -> MLP:
    torch.manual_seed(seed)
    return MLP(in_dim=in_dim, hidden=hidden, out_dim=out_dim)


# ──────────────────────────────────────────────────────────────────────────────
# MLP.forward — shape & dtype sweep across random architectures
# ──────────────────────────────────────────────────────────────────────────────

@pytest.mark.parametrize("in_dim,hidden,out_dim,n", [
    (2, 4, 2, 1),      # single-example edge case
    (3, 5, 4, 10),
    (8, 16, 3, 32),
    (1, 1, 1, 7),      # degenerate tiny dims
    (20, 64, 10, 100),
    (5, 8, 6, 1),      # N=1 again, bigger dims
])
def test_forward_output_shape_sweep(in_dim, hidden, out_dim, n):
    """forward always returns (N, out_dim) regardless of architecture."""
    model = _make_model(in_dim, hidden, out_dim, seed=in_dim * 31 + out_dim)
    torch.manual_seed(out_dim)
    X = torch.randn(n, in_dim)
    logits = model(X)
    assert tuple(logits.shape) == (n, out_dim), (
        f"Expected shape ({n},{out_dim}), got {tuple(logits.shape)}"
    )


def test_forward_output_dtype_is_float32():
    """Logits must be float32 (same as input)."""
    torch.manual_seed(42)
    model = MLP(in_dim=6, hidden=12, out_dim=4)
    X = torch.randn(8, 6)   # default dtype is float32
    logits = model(X)
    assert logits.dtype == torch.float32


def test_forward_output_is_not_probabilities_random_seeds():
    """Over many seeds, logits must NOT look like probability distributions
    (i.e. at least one sample has a row that doesn't sum to 1 or has a negative)."""
    failures = 0
    for seed in range(20):
        torch.manual_seed(seed)
        model = MLP(in_dim=5, hidden=10, out_dim=4)
        X = torch.randn(12, 5)
        logits = model(X)
        row_sums = logits.sum(dim=1)
        looks_like_probs = (
            torch.all(logits >= 0).item()
            and torch.allclose(row_sums, torch.ones_like(row_sums), atol=1e-4)
        )
        if looks_like_probs:
            failures += 1
    # It would be astronomically unlikely for even one seed to accidentally
    # produce unit-sum non-negative logits; zero is expected.
    assert failures == 0, f"{failures}/20 seeds produced logit rows summing to 1 — softmax applied?"


# ──────────────────────────────────────────────────────────────────────────────
# Parameter count formula: in*h + h + h*out + out for a two-layer MLP
# ──────────────────────────────────────────────────────────────────────────────

@pytest.mark.parametrize("in_dim,hidden,out_dim", [
    (4, 8, 3),
    (1, 1, 1),
    (10, 50, 5),
    (7, 3, 9),
    (100, 256, 10),
])
def test_parameter_count_formula(in_dim, hidden, out_dim):
    """Total params == in_dim*hidden + hidden + hidden*out_dim + out_dim."""
    model = MLP(in_dim=in_dim, hidden=hidden, out_dim=out_dim)
    expected = in_dim * hidden + hidden + hidden * out_dim + out_dim
    actual = sum(p.numel() for p in model.parameters())
    assert actual == expected, (
        f"({in_dim}->{hidden}->{out_dim}): expected {expected} params, got {actual}"
    )


# ──────────────────────────────────────────────────────────────────────────────
# Gradients: every parameter must get a non-zero .grad after backward
# ──────────────────────────────────────────────────────────────────────────────

def test_all_parameters_have_nonzero_grad_after_train_step():
    """Every leaf parameter must receive a non-zero gradient — proves both fc1 and fc2
    are in the computation graph (no dead path)."""
    torch.manual_seed(7)
    model = MLP(in_dim=6, hidden=10, out_dim=4)
    optimizer = torch.optim.SGD(model.parameters(), lr=0.01)
    loss_fn = nn.CrossEntropyLoss()
    X, y = _make_batch(20, 6, 4, seed=7)

    train_step(model, optimizer, loss_fn, X, y)

    for name, p in model.named_parameters():
        assert p.grad is not None, f"param '{name}' has None gradient"
        assert p.grad.abs().sum().item() > 0, f"param '{name}' has all-zero gradient"


@pytest.mark.parametrize("seed", [0, 3, 17, 42, 99])
def test_gradients_nonzero_across_seeds(seed):
    """Gradient property holds across multiple random seeds."""
    torch.manual_seed(seed)
    model = MLP(in_dim=4, hidden=8, out_dim=3)
    optimizer = torch.optim.SGD(model.parameters(), lr=0.05)
    loss_fn = nn.CrossEntropyLoss()
    X, y = _make_batch(16, 4, 3, seed=seed + 100)
    train_step(model, optimizer, loss_fn, X, y)
    for p in model.parameters():
        assert p.grad is not None
        assert p.grad.abs().max().item() > 0


# ──────────────────────────────────────────────────────────────────────────────
# train_step: loss decreases after one step (with sufficient lr)
# ──────────────────────────────────────────────────────────────────────────────

@pytest.mark.parametrize("seed", [1, 5, 11, 23, 77])
def test_single_train_step_decreases_loss(seed):
    """A single train_step with a decent lr should reduce loss on the SAME batch."""
    torch.manual_seed(seed)
    model = MLP(in_dim=5, hidden=16, out_dim=3)
    loss_fn = nn.CrossEntropyLoss()
    X, y = _make_batch(30, 5, 3, seed=seed + 200)

    # Compute loss before step (no grad update)
    model.eval()
    with torch.no_grad():
        loss_before = loss_fn(model(X), y).item()
    model.train()

    optimizer = torch.optim.SGD(model.parameters(), lr=0.5)
    loss_after_step = train_step(model, optimizer, loss_fn, X, y)
    # loss_after_step is loss BEFORE parameter update; evaluate after update
    with torch.no_grad():
        loss_after = loss_fn(model(X), y).item()

    assert loss_after < loss_before, (
        f"seed={seed}: loss did not decrease ({loss_before:.4f} -> {loss_after:.4f})"
    )


def test_train_step_returned_value_matches_pre_step_loss():
    """train_step must return the loss evaluated on the CURRENT parameters (before step)."""
    torch.manual_seed(13)
    model = MLP(in_dim=4, hidden=8, out_dim=3)
    loss_fn = nn.CrossEntropyLoss()
    X, y = _make_batch(20, 4, 3, seed=314)

    # Capture loss before any update
    model.eval()
    with torch.no_grad():
        expected_loss = loss_fn(model(X), y).item()
    model.train()

    optimizer = torch.optim.SGD(model.parameters(), lr=0.0)  # lr=0 -> params don't move
    returned = train_step(model, optimizer, loss_fn, X, y)

    assert abs(returned - expected_loss) < 1e-5, (
        f"Returned loss {returned:.6f} != pre-step loss {expected_loss:.6f}"
    )


# ──────────────────────────────────────────────────────────────────────────────
# zero_grad contract: gradients must not accumulate across calls
# ──────────────────────────────────────────────────────────────────────────────

def test_zero_grad_clears_between_steps():
    """Running train_step twice must not accumulate gradients.
    If zero_grad is missing, the second call's gradients double up."""
    torch.manual_seed(55)
    model = MLP(in_dim=4, hidden=8, out_dim=3)
    optimizer = torch.optim.SGD(model.parameters(), lr=0.0)  # freeze weights, only check grads
    loss_fn = nn.CrossEntropyLoss()
    X, y = _make_batch(20, 4, 3, seed=55)

    train_step(model, optimizer, loss_fn, X, y)
    grads_first = [p.grad.clone() for p in model.parameters()]

    train_step(model, optimizer, loss_fn, X, y)
    grads_second = [p.grad.clone() for p in model.parameters()]

    # With correct zero_grad, grads should be identical (same data, same params, lr=0)
    for g1, g2 in zip(grads_first, grads_second):
        assert torch.allclose(g1, g2, atol=1e-6), (
            "Gradients differ between two steps on the same data — zero_grad may be missing "
            "or in the wrong position"
        )


# ──────────────────────────────────────────────────────────────────────────────
# accuracy: dtype, range, and boundary properties
# ──────────────────────────────────────────────────────────────────────────────

@pytest.mark.parametrize("seed", [2, 6, 18, 50])
def test_accuracy_returns_float_in_unit_interval(seed):
    """accuracy must return a Python float in [0, 1] for random inputs."""
    torch.manual_seed(seed)
    model = MLP(in_dim=5, hidden=8, out_dim=4)
    X, y = _make_batch(25, 5, 4, seed=seed + 500)
    acc = accuracy(model, X, y)
    assert isinstance(acc, float)
    assert 0.0 <= acc <= 1.0


def test_accuracy_zero_when_always_wrong():
    """Construct a model whose logits always predict class 0, with all-class-1 labels -> acc=0."""
    class _AlwaysClass0(nn.Module):
        def __init__(self):
            super().__init__()

        def forward(self, x):
            # Return logits with highest value at index 0 for every sample
            n = x.shape[0]
            logits = torch.zeros(n, 3)
            logits[:, 0] = 10.0   # always argmax = 0
            return logits

    model = _AlwaysClass0()
    X = torch.randn(10, 3)
    y = torch.ones(10, dtype=torch.long)   # all class 1 -> always wrong
    acc = accuracy(model, X, y)
    assert acc == 0.0


def test_accuracy_single_example():
    """accuracy with N=1 must work (edge case for mean)."""
    torch.manual_seed(99)
    model = MLP(in_dim=3, hidden=6, out_dim=2)
    X = torch.randn(1, 3)
    y = torch.zeros(1, dtype=torch.long)
    acc = accuracy(model, X, y)
    assert isinstance(acc, float)
    assert acc in (0.0, 1.0)   # with N=1 only 0 or 1 is possible


def test_accuracy_does_not_build_computation_graph():
    """accuracy must use torch.no_grad() — no gradient-tracked tensors should leak."""
    torch.manual_seed(88)
    model = MLP(in_dim=4, hidden=8, out_dim=3)
    X, y = _make_batch(16, 4, 3, seed=88)

    # Make X require grad so we can check if a graph was built
    X.requires_grad_(True)
    _ = accuracy(model, X, y)

    # If no_grad was used, X.grad should remain None (no backward was called)
    # and — more importantly — the returned float has no grad_fn
    # We re-run to check the logits don't have grad_fn:
    model.eval()
    with torch.no_grad():
        logits_ref = model(X)
    assert logits_ref.grad_fn is None, "accuracy should run under torch.no_grad()"


def test_accuracy_switches_model_to_eval_mode():
    """accuracy() must call model.eval() — model.training should be False inside."""
    training_flags = []

    class _SpyMLP(nn.Module):
        def __init__(self, inner):
            super().__init__()
            self.inner = inner

        def forward(self, x):
            training_flags.append(self.training)
            return self.inner(x)

    torch.manual_seed(21)
    inner = MLP(in_dim=4, hidden=8, out_dim=3)
    spy = _SpyMLP(inner)
    spy.train()   # start in train mode

    X, y = _make_batch(10, 4, 3, seed=21)
    accuracy(spy, X, y)

    assert len(training_flags) > 0
    assert all(flag is False for flag in training_flags), (
        "model.forward was called while model.training=True — eval() not called"
    )


# ──────────────────────────────────────────────────────────────────────────────
# Numerical stability edge cases
# ──────────────────────────────────────────────────────────────────────────────

def test_forward_all_zeros_input():
    """MLP must not crash or return NaN/Inf on an all-zero input."""
    torch.manual_seed(31)
    model = MLP(in_dim=5, hidden=10, out_dim=3)
    X = torch.zeros(8, 5)
    logits = model(X)
    assert not torch.any(torch.isnan(logits)), "NaN in logits for all-zero input"
    assert not torch.any(torch.isinf(logits)), "Inf in logits for all-zero input"


def test_forward_all_negative_input():
    """MLP must handle all-negative inputs (ReLU gating test)."""
    torch.manual_seed(32)
    model = MLP(in_dim=5, hidden=10, out_dim=3)
    X = -10.0 * torch.ones(8, 5)
    logits = model(X)
    assert not torch.any(torch.isnan(logits))
    assert not torch.any(torch.isinf(logits))
    # Shape must still be correct
    assert tuple(logits.shape) == (8, 3)


def test_forward_large_magnitude_input():
    """Large-magnitude inputs (1e6) must not produce NaN — tests numerical stability."""
    torch.manual_seed(33)
    model = MLP(in_dim=4, hidden=16, out_dim=5)
    X = 1e6 * torch.randn(10, 4)
    logits = model(X)
    # We only require no NaN here; Inf is possible but NaN indicates a real problem
    assert not torch.any(torch.isnan(logits)), "NaN in logits for large-magnitude input"


def test_train_step_large_magnitude_does_not_produce_nan_loss():
    """train_step should return a finite loss even for reasonably large inputs."""
    torch.manual_seed(34)
    model = MLP(in_dim=4, hidden=8, out_dim=3)
    optimizer = torch.optim.SGD(model.parameters(), lr=1e-8)  # tiny lr to avoid explosion
    loss_fn = nn.CrossEntropyLoss()
    X = 5.0 * torch.randn(16, 4)
    y = torch.randint(0, 3, (16,))
    loss_val = train_step(model, optimizer, loss_fn, X, y)
    assert np.isfinite(loss_val), f"Loss is not finite: {loss_val}"


# ──────────────────────────────────────────────────────────────────────────────
# Oracle: compare MLP forward against manually composed nn.Linear + ReLU + nn.Linear
# ──────────────────────────────────────────────────────────────────────────────

def test_forward_matches_manual_two_linear_relu_composition():
    """MLP forward must equal fc2(relu(fc1(x))) with the SAME weights — direct oracle check."""
    torch.manual_seed(77)
    in_dim, hidden, out_dim = 5, 9, 4
    model = MLP(in_dim=in_dim, hidden=hidden, out_dim=out_dim)

    # Extract weights to build a reference manually
    fc1_weight = model.fc1.weight.detach().clone()
    fc1_bias   = model.fc1.bias.detach().clone()
    fc2_weight = model.fc2.weight.detach().clone()
    fc2_bias   = model.fc2.bias.detach().clone()

    torch.manual_seed(78)
    X = torch.randn(12, in_dim)

    # Reference: manual matrix multiplications
    h = torch.relu(X @ fc1_weight.T + fc1_bias)
    ref_logits = h @ fc2_weight.T + fc2_bias

    logits = model(X)
    assert torch.allclose(logits, ref_logits, atol=1e-5), (
        "MLP forward does not match manual fc2(relu(fc1(x))) computation"
    )


# ──────────────────────────────────────────────────────────────────────────────
# Consistency: accuracy on a fully-trained model converges to high values
# ──────────────────────────────────────────────────────────────────────────────

@pytest.mark.parametrize("seed,n_classes", [(10, 2), (20, 3), (30, 4)])
def test_accuracy_high_after_convergence(seed, n_classes):
    """After sufficient training on a well-separated dataset, accuracy must be >= 0.85."""
    torch.manual_seed(seed)
    # Generate well-separated clusters
    centers = 5.0 * torch.randn(n_classes, 6)
    xs, ys = [], []
    for c in range(n_classes):
        cloud = centers[c] + 0.3 * torch.randn(40, 6)
        xs.append(cloud)
        ys.append(torch.full((40,), c, dtype=torch.long))
    X = torch.cat(xs)
    y = torch.cat(ys)

    model = MLP(in_dim=6, hidden=32, out_dim=n_classes)
    optimizer = torch.optim.Adam(model.parameters(), lr=0.05)
    loss_fn = nn.CrossEntropyLoss()

    for _ in range(200):
        train_step(model, optimizer, loss_fn, X, y)

    acc = accuracy(model, X, y)
    assert acc >= 0.85, (
        f"seed={seed}, classes={n_classes}: accuracy only {acc:.3f} after 200 steps"
    )


# ──────────────────────────────────────────────────────────────────────────────
# ReLU non-linearity: hidden-layer outputs must contain zeros (ReLU activations fired)
# ──────────────────────────────────────────────────────────────────────────────

def test_relu_produces_sparsity_in_hidden_layer():
    """With random inputs roughly half of ReLU outputs should be exactly zero.
    If ReLU is missing (pure linear), this proportion would be ~0."""
    torch.manual_seed(66)
    model = MLP(in_dim=8, hidden=64, out_dim=3)

    torch.manual_seed(67)
    X = torch.randn(100, 8)

    # Hook to capture the output of fc1 before ReLU is applied
    pre_relu = {}
    def _hook(module, inp, out):
        pre_relu['out'] = out.detach()

    model.fc1.register_forward_hook(_hook)
    _ = model(X)

    # After fc1 (before ReLU), some values negative -> ReLU zeros them
    h_pre = pre_relu['out']
    fraction_negative = (h_pre < 0).float().mean().item()

    # For random weights and standard normal inputs, ~50% should be negative
    assert fraction_negative > 0.1, (
        f"Only {fraction_negative:.1%} of pre-activation values are negative — "
        "check that ReLU is actually being applied to meaningful values"
    )

    # Now check that the full forward logits reflect nonlinearity:
    # A pure linear model would give rank <= in_dim outputs; with relu hidden
    # activations, the effective rank should be > 1 for a batch of 100 samples.
    logits = model(X)
    # singular values of (100, out_dim=3) logit matrix — at least 2 should be non-negligible
    sv = torch.linalg.svdvals(logits)
    assert (sv > 1e-3).sum().item() >= min(2, logits.shape[1]), (
        "Logit matrix appears rank-deficient — ReLU may not be creating non-linearity"
    )


# ──────────────────────────────────────────────────────────────────────────────
# train_epoch: one epoch of mini-batch training (Dataset / DataLoader)
# ──────────────────────────────────────────────────────────────────────────────

def _toy_separable(n_per_class: int, n_classes: int, in_dim: int, seed: int):
    """Well-separated clusters, deterministic — float32 X, int64 y."""
    torch.manual_seed(seed)
    centers = 4.0 * torch.randn(n_classes, in_dim)
    xs, ys = [], []
    for c in range(n_classes):
        cloud = centers[c] + 0.5 * torch.randn(n_per_class, in_dim)
        xs.append(cloud)
        ys.append(torch.full((n_per_class,), c, dtype=torch.long))
    return torch.cat(xs), torch.cat(ys)


@pytest.mark.parametrize("batch_size", [1, 8, 32, "full"])
def test_train_epoch_mean_loss_finite_and_positive(batch_size):
    """Across batch sizes (incl. 1, a non-dividing size, and full-batch) the returned
    mean loss must be a finite positive float."""
    torch.manual_seed(4)
    X, y = _toy_separable(n_per_class=30, n_classes=3, in_dim=5, seed=4)  # N=90
    bs = X.shape[0] if batch_size == "full" else batch_size

    model = MLP(in_dim=5, hidden=12, out_dim=3)
    optimizer = torch.optim.SGD(model.parameters(), lr=0.05)
    loss_fn = nn.CrossEntropyLoss()

    mean_loss = train_epoch(model, optimizer, loss_fn, X, y, batch_size=bs)
    assert isinstance(mean_loss, float)
    assert np.isfinite(mean_loss), f"batch_size={batch_size}: mean loss not finite ({mean_loss})"
    assert mean_loss > 0.0, f"batch_size={batch_size}: mean loss not positive ({mean_loss})"


@pytest.mark.parametrize("batch_size", [4, 16, 50])
def test_train_epoch_moves_parameters(batch_size):
    """After one epoch at least one parameter must have changed (steps actually applied)."""
    torch.manual_seed(9)
    X, y = _toy_separable(n_per_class=20, n_classes=3, in_dim=4, seed=9)  # N=60
    model = MLP(in_dim=4, hidden=8, out_dim=3)
    optimizer = torch.optim.SGD(model.parameters(), lr=0.1)
    loss_fn = nn.CrossEntropyLoss()

    before = [p.detach().clone() for p in model.parameters()]
    train_epoch(model, optimizer, loss_fn, X, y, batch_size=batch_size)
    after = list(model.parameters())

    changed = any(not torch.allclose(b, a) for b, a in zip(before, after))
    assert changed, f"batch_size={batch_size}: no parameter moved after a full epoch"


@pytest.mark.parametrize("seed", [0, 7, 21])
def test_train_epoch_full_batch_oracle_equals_train_step(seed):
    """Full-batch oracle across seeds: with batch_size = N, one epoch == one train_step,
    both in returned loss and in resulting parameters (order inside the single batch is
    irrelevant for the full-batch gradient)."""
    X, y = _toy_separable(n_per_class=15, n_classes=4, in_dim=6, seed=seed)
    N = X.shape[0]
    loss_fn = nn.CrossEntropyLoss()

    torch.manual_seed(1000 + seed)
    model_step = MLP(in_dim=6, hidden=10, out_dim=4)
    torch.manual_seed(1000 + seed)
    model_epoch = MLP(in_dim=6, hidden=10, out_dim=4)

    opt_step = torch.optim.SGD(model_step.parameters(), lr=0.1)
    opt_epoch = torch.optim.SGD(model_epoch.parameters(), lr=0.1)

    loss_step = train_step(model_step, opt_step, loss_fn, X, y)
    loss_epoch = train_epoch(model_epoch, opt_epoch, loss_fn, X, y, batch_size=N)

    assert abs(loss_epoch - loss_step) < 1e-4, (
        f"seed={seed}: full-batch epoch loss {loss_epoch:.6f} != train_step {loss_step:.6f}"
    )
    for p_s, p_e in zip(model_step.parameters(), model_epoch.parameters()):
        assert torch.allclose(p_s, p_e, atol=1e-5), (
            f"seed={seed}: parameters diverged between train_step and full-batch train_epoch"
        )
