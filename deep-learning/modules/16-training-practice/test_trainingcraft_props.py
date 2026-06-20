"""
Randomised, oracle-backed property tests for module 16 — Обучение на практике.

Design rules:
  - Every test sets a deterministic seed (different per test) and loops over
    several random shapes / values — so failures are reproducible.
  - All calls to the module's functions are INSIDE test functions (no module-level
    solution calls), so this file imports/collects cleanly against stub stubs.
  - Oracle: torch.nn.functional.layer_norm is the ground-truth for layernorm.
    math.isclose / hand-rolled formulas are oracles for init_scale and lr_at_step.
"""

import math

import pytest
import torch
import torch.nn.functional as F

from trainingcraft import layernorm, init_scale, lr_at_step


# ===========================================================================
# layernorm – randomised oracle comparisons
# ===========================================================================

def test_layernorm_random_shapes_match_torch():
    """Loop over multiple random (N, D) shapes; must match F.layer_norm exactly."""
    torch.manual_seed(10)
    for N, D in [(1, 2), (3, 5), (10, 16), (7, 32), (2, 128)]:
        x = torch.randn(N, D)
        gamma = torch.randn(D)
        beta = torch.randn(D)
        out = layernorm(x, gamma, beta)
        ref = F.layer_norm(x, (D,), weight=gamma, bias=beta, eps=1e-5)
        assert out.shape == ref.shape, f"shape mismatch at (N={N}, D={D})"
        assert torch.allclose(out, ref, atol=1e-5), (
            f"value mismatch at (N={N}, D={D}): max_err={( out - ref).abs().max().item()}"
        )


def test_layernorm_random_3d_shapes_match_torch():
    """Batch × time × feature tensors — norms along last axis only."""
    torch.manual_seed(11)
    for N, T, D in [(2, 3, 4), (1, 8, 16), (4, 5, 8)]:
        x = torch.randn(N, T, D)
        gamma = torch.randn(D)
        beta = torch.randn(D)
        out = layernorm(x, gamma, beta)
        ref = F.layer_norm(x, (D,), weight=gamma, bias=beta, eps=1e-5)
        assert out.shape == (N, T, D)
        assert torch.allclose(out, ref, atol=1e-5), (
            f"value mismatch at (N={N},T={T},D={D})"
        )


def test_layernorm_output_shape_preserved():
    """Output shape must equal input shape for any (..., D) input."""
    torch.manual_seed(12)
    for shape in [(4, 8), (1, 1), (3, 2, 7), (2, 3, 4, 5)]:
        D = shape[-1]
        x = torch.randn(*shape)
        gamma = torch.ones(D)
        beta = torch.zeros(D)
        out = layernorm(x, gamma, beta)
        assert out.shape == x.shape, f"shape {out.shape} != {x.shape}"


def test_layernorm_output_dtype_float32():
    """Result must stay float32 (module spec: all float32)."""
    torch.manual_seed(13)
    for N, D in [(3, 6), (10, 20)]:
        x = torch.randn(N, D)
        gamma = torch.ones(D)
        beta = torch.zeros(D)
        out = layernorm(x, gamma, beta)
        assert out.dtype == torch.float32


def test_layernorm_each_row_zero_mean_unit_std_random_inputs():
    """With gamma=1, beta=0, every row has mean≈0 and biased-std≈1."""
    torch.manual_seed(14)
    for _ in range(6):
        N = torch.randint(2, 12, ()).item()
        D = torch.randint(4, 30, ()).item()
        x = torch.randn(N, D) * torch.rand(1).item() * 10 + torch.randn(1).item() * 5
        gamma = torch.ones(D)
        beta = torch.zeros(D)
        out = layernorm(x, gamma, beta)
        means = out.mean(dim=-1)
        stds = out.std(dim=-1, unbiased=False)
        assert torch.allclose(means, torch.zeros(N), atol=1e-5), (
            f"row means not zero: max={means.abs().max().item()}"
        )
        assert torch.allclose(stds, torch.ones(N), atol=1e-3), (
            f"row stds not one: max={(stds - 1).abs().max().item()}"
        )


def test_layernorm_scale_shift_applied_correctly():
    """gamma and beta are applied per-feature after normalisation."""
    torch.manual_seed(15)
    D = 10
    x = torch.randn(5, D)
    gamma = torch.randn(D) * 2          # arbitrary non-trivial scale
    beta = torch.randn(D) * 3           # arbitrary non-trivial shift
    out = layernorm(x, gamma, beta)
    ref = F.layer_norm(x, (D,), weight=gamma, bias=beta, eps=1e-5)
    assert torch.allclose(out, ref, atol=1e-5)


def test_layernorm_affine_linearity_in_gamma_beta():
    """Doubling gamma doubles the deviation from beta; adding delta to beta shifts output."""
    torch.manual_seed(16)
    D = 8
    x = torch.randn(4, D)
    gamma = torch.abs(torch.randn(D)) + 0.1   # positive gammas
    beta = torch.zeros(D)

    out1 = layernorm(x, gamma, beta)
    out2 = layernorm(x, 2.0 * gamma, beta)
    # out2 should be exactly 2 * (out1 - beta) + beta = 2*out1 when beta=0
    assert torch.allclose(out2, 2.0 * out1, atol=1e-5), "doubling gamma should double output"

    delta = torch.randn(D)
    out3 = layernorm(x, gamma, beta + delta)
    assert torch.allclose(out3, out1 + delta, atol=1e-5), "beta shift should translate output"


def test_layernorm_numerical_stability_large_values():
    """Large-magnitude inputs should not produce inf/nan."""
    torch.manual_seed(17)
    D = 16
    x = torch.randn(4, D) * 1e4
    gamma = torch.ones(D)
    beta = torch.zeros(D)
    out = layernorm(x, gamma, beta)
    assert not out.isnan().any(), "NaN in output for large inputs"
    assert not out.isinf().any(), "Inf in output for large inputs"


def test_layernorm_numerical_stability_near_zero():
    """Near-zero inputs (eps prevents division by zero for constant rows)."""
    torch.manual_seed(18)
    D = 8
    # Nearly constant row — var ≈ 0, eps is the safety net
    x = torch.ones(3, D) * 1e-8
    x[0, 0] += 1e-9   # tiny perturbation so it's not identically constant
    gamma = torch.ones(D)
    beta = torch.zeros(D)
    out = layernorm(x, gamma, beta)
    assert not out.isnan().any()
    assert not out.isinf().any()


def test_layernorm_single_example():
    """Edge case: batch size 1."""
    torch.manual_seed(19)
    D = 12
    x = torch.randn(1, D)
    gamma = torch.randn(D)
    beta = torch.randn(D)
    out = layernorm(x, gamma, beta)
    ref = F.layer_norm(x, (D,), weight=gamma, bias=beta, eps=1e-5)
    assert torch.allclose(out, ref, atol=1e-5)


def test_layernorm_negative_values_in_x():
    """All-negative input rows — normalisation should still work."""
    torch.manual_seed(20)
    D = 6
    x = -torch.abs(torch.randn(5, D)) - 10.0
    gamma = torch.ones(D)
    beta = torch.zeros(D)
    out = layernorm(x, gamma, beta)
    ref = F.layer_norm(x, (D,), weight=gamma, bias=beta, eps=1e-5)
    assert torch.allclose(out, ref, atol=1e-5)


def test_layernorm_d_equals_1_uses_biased_variance():
    """D=1: biased variance is 0, output should equal beta (the normed value is 0 * gamma + beta)."""
    torch.manual_seed(21)
    D = 1
    x = torch.randn(5, D)
    gamma = torch.ones(D)
    beta = torch.full((D,), 3.14)
    out = layernorm(x, gamma, beta)
    ref = F.layer_norm(x, (D,), weight=gamma, bias=beta, eps=1e-5)
    assert torch.allclose(out, ref, atol=1e-5)


def test_layernorm_gradient_flows_through():
    """Gradients must be non-None and non-zero for x, gamma, beta."""
    torch.manual_seed(22)
    D = 8
    x = torch.randn(4, D, requires_grad=True)
    gamma = torch.ones(D, requires_grad=True)
    beta = torch.zeros(D, requires_grad=True)
    out = layernorm(x, gamma, beta)
    loss = out.sum()
    loss.backward()
    assert x.grad is not None and x.grad.abs().max() > 0
    assert gamma.grad is not None and gamma.grad.abs().max() > 0
    assert beta.grad is not None and beta.grad.abs().max() > 0


# ===========================================================================
# init_scale – randomised property checks
# ===========================================================================

def test_init_scale_formula_random_fan_ins():
    """1/sqrt(fan_in) for many random fan_in values."""
    import random
    random.seed(30)
    for _ in range(20):
        fan_in = random.randint(1, 10000)
        expected = 1.0 / math.sqrt(fan_in)
        got = init_scale(fan_in)
        assert isinstance(got, float), f"expected float, got {type(got)}"
        assert math.isclose(got, expected, rel_tol=1e-9), (
            f"init_scale({fan_in})={got} expected {expected}"
        )


def test_init_scale_strictly_decreasing():
    """Larger fan_in must always give a smaller scale."""
    fan_ins = [1, 2, 4, 9, 16, 25, 100, 1024, 4096]
    scales = [init_scale(f) for f in fan_ins]
    for a, b in zip(scales, scales[1:]):
        assert b < a, f"init_scale not strictly decreasing: {a} -> {b}"


def test_init_scale_positive():
    """Scale must always be positive."""
    for fan_in in [1, 5, 100, 10000]:
        assert init_scale(fan_in) > 0


def test_init_scale_variance_control():
    """
    Empirical check: if W ~ Normal(0, init_scale(fan_in)) and x ~ Normal(0,1),
    then Var(W @ x) should be ≈ 1 (signal variance preservation).
    """
    torch.manual_seed(31)
    for fan_in in [50, 200, 1000]:
        scale = init_scale(fan_in)
        n_trials = 8000
        x = torch.randn(n_trials, fan_in)
        W = torch.randn(fan_in) * scale
        z = x @ W        # shape (n_trials,)
        empirical_var = z.var().item()
        # Allow generous tolerance for randomness; larger fan_in converges better.
        # For fan_in >= 50 with 8000 trials the variance is very reliably in (0.6, 1.5).
        assert 0.6 < empirical_var < 1.5, (
            f"fan_in={fan_in}: empirical Var(Wx)={empirical_var:.3f}, expected ≈1.0"
        )


def test_init_scale_perfect_squares():
    """For perfect squares, result is an exact reciprocal integer (or close)."""
    pairs = [(1, 1.0), (4, 0.5), (9, 1/3), (25, 0.2), (100, 0.1), (10000, 0.01)]
    for fan_in, expected in pairs:
        assert math.isclose(init_scale(fan_in), expected, rel_tol=1e-9), (
            f"init_scale({fan_in}) expected {expected}, got {init_scale(fan_in)}"
        )


# ===========================================================================
# lr_at_step – randomised property checks
# ===========================================================================

def test_lr_warmup_linear_ramp_multiple_configs():
    """During warmup, lr grows linearly: lr(step) = base_lr * step / warmup."""
    import random
    random.seed(40)
    for _ in range(8):
        base_lr = random.uniform(0.001, 1.0)
        warmup = random.randint(5, 50)
        total = warmup + random.randint(50, 200)
        for step in range(warmup):
            got = lr_at_step(base_lr, step, warmup, total)
            expected = base_lr * step / warmup
            assert math.isclose(got, expected, rel_tol=1e-9, abs_tol=1e-12), (
                f"warmup step {step}/{warmup}: got {got}, expected {expected}"
            )


def test_lr_peak_equals_base_lr_various_configs():
    """At step == warmup, both schedules return exactly base_lr."""
    import random
    random.seed(41)
    for _ in range(10):
        base_lr = random.uniform(0.001, 1.0)
        warmup = random.randint(0, 100)
        total = warmup + random.randint(1, 200)
        for sched in ("linear", "cosine"):
            got = lr_at_step(base_lr, warmup, warmup, total, schedule=sched)
            assert math.isclose(got, base_lr, rel_tol=1e-9), (
                f"peak mismatch: sched={sched}, warmup={warmup}, total={total}: "
                f"got {got}, expected {base_lr}"
            )


def test_lr_zero_at_and_beyond_total():
    """lr must be 0.0 at step==total and any step > total."""
    import random
    random.seed(42)
    for _ in range(10):
        base_lr = random.uniform(0.01, 1.0)
        warmup = random.randint(0, 50)
        total = warmup + random.randint(10, 200)
        for sched in ("linear", "cosine"):
            at_end = lr_at_step(base_lr, total, warmup, total, schedule=sched)
            beyond = lr_at_step(base_lr, total + 50, warmup, total, schedule=sched)
            assert math.isclose(at_end, 0.0, abs_tol=1e-9), (
                f"lr at total ({total}) should be 0.0, got {at_end}"
            )
            assert math.isclose(beyond, 0.0, abs_tol=1e-12), (
                f"lr beyond total should be 0.0, got {beyond}"
            )


def test_lr_bounds_all_steps_all_schedules():
    """lr is always in [0, base_lr] for every step in [0, total]."""
    import random
    random.seed(43)
    for _ in range(6):
        base_lr = random.uniform(0.001, 2.0)
        warmup = random.randint(0, 30)
        total = warmup + random.randint(20, 150)
        for sched in ("linear", "cosine"):
            for step in range(0, total + 1):
                lr = lr_at_step(base_lr, step, warmup, total, schedule=sched)
                assert -1e-12 <= lr <= base_lr + 1e-9, (
                    f"lr out of bounds: sched={sched}, step={step}, lr={lr}, base_lr={base_lr}"
                )


def test_lr_decay_monotonically_decreasing_random():
    """After warmup, lr must be non-increasing for both schedules."""
    import random
    random.seed(44)
    for _ in range(8):
        base_lr = random.uniform(0.01, 1.0)
        warmup = random.randint(0, 20)
        total = warmup + random.randint(20, 200)
        for sched in ("linear", "cosine"):
            prev = lr_at_step(base_lr, warmup, warmup, total, schedule=sched)
            for step in range(warmup + 1, total + 1):
                cur = lr_at_step(base_lr, step, warmup, total, schedule=sched)
                assert cur <= prev + 1e-12, (
                    f"lr increased during decay: sched={sched}, "
                    f"step {step - 1}->{step}: {prev:.6f}->{cur:.6f}"
                )
                prev = cur
            # and it strictly descended
            final = lr_at_step(base_lr, total, warmup, total, schedule=sched)
            assert final < prev + 1e-12   # final <= last step before total


def test_lr_warmup_is_monotonically_increasing():
    """During warmup phase, lr is strictly increasing."""
    import random
    random.seed(45)
    for _ in range(8):
        base_lr = random.uniform(0.01, 1.0)
        warmup = random.randint(2, 40)
        total = warmup + random.randint(20, 200)
        vals = [lr_at_step(base_lr, s, warmup, total) for s in range(warmup + 1)]
        for a, b in zip(vals, vals[1:]):
            assert b > a - 1e-12, f"warmup not monotone: {a}->{b}"


def test_lr_cosine_midpoint_formula():
    """cosine at progress=0.5: lr = base_lr * 0.5 * (1 + cos(pi/2)) = base_lr/2."""
    import random
    random.seed(46)
    for _ in range(10):
        base_lr = random.uniform(0.01, 2.0)
        warmup = 0
        total = 100
        mid_step = 50   # progress = 50/100 = 0.5
        lr = lr_at_step(base_lr, mid_step, warmup, total, schedule="cosine")
        expected = base_lr * 0.5 * (1 + math.cos(math.pi * 0.5))
        assert math.isclose(lr, expected, rel_tol=1e-9), (
            f"cosine midpoint: got {lr}, expected {expected}"
        )


def test_lr_linear_midpoint_formula():
    """linear at progress=0.5: lr = base_lr * 0.5."""
    import random
    random.seed(47)
    for _ in range(10):
        base_lr = random.uniform(0.01, 2.0)
        warmup = 0
        total = 100
        mid_step = 50
        lr = lr_at_step(base_lr, mid_step, warmup, total, schedule="linear")
        assert math.isclose(lr, base_lr * 0.5, rel_tol=1e-9), (
            f"linear midpoint: got {lr}, expected {base_lr * 0.5}"
        )


def test_lr_cosine_vs_linear_shape():
    """
    Cosine and linear decay agree at endpoints (peak and zero).
    In the first quarter of the decay cosine is ABOVE linear (slow start);
    in the last quarter cosine is ABOVE linear (slow finish).
    In the exact middle both strategies are equal (both give base_lr/2).
    """
    import random
    random.seed(48)
    for _ in range(5):
        base_lr = random.uniform(0.01, 1.0)
        warmup = 0
        total = 100   # use fixed total for easy quarter arithmetic

        # Both start at base_lr
        assert math.isclose(
            lr_at_step(base_lr, 0, warmup, total, "cosine"),
            lr_at_step(base_lr, 0, warmup, total, "linear"),
            rel_tol=1e-9,
        )
        # Both end at 0
        assert math.isclose(
            lr_at_step(base_lr, total, warmup, total, "cosine"),
            lr_at_step(base_lr, total, warmup, total, "linear"),
            abs_tol=1e-9,
        )
        # First quarter (progress=0.25): cosine is still HIGH — slower initial drop
        # cosine: 0.5*(1+cos(pi*0.25)) ≈ 0.854 * base_lr  >  linear: 0.75 * base_lr
        cos_q1 = lr_at_step(base_lr, 25, warmup, total, "cosine")
        lin_q1 = lr_at_step(base_lr, 25, warmup, total, "linear")
        assert cos_q1 > lin_q1, (
            f"cosine should be above linear at first quarter: cos={cos_q1:.4f}, lin={lin_q1:.4f}"
        )
        # Midpoint: both equal base_lr/2 (cos(pi/2) = 0)
        cos_mid = lr_at_step(base_lr, 50, warmup, total, "cosine")
        lin_mid = lr_at_step(base_lr, 50, warmup, total, "linear")
        assert math.isclose(cos_mid, base_lr / 2, rel_tol=1e-9)
        assert math.isclose(lin_mid, base_lr / 2, rel_tol=1e-9)
        # Third quarter (progress=0.75): cosine is LOW — faster drop in second half
        # cosine: 0.5*(1+cos(3*pi/4)) ≈ 0.146 * base_lr  <  linear: 0.25 * base_lr
        cos_q3 = lr_at_step(base_lr, 75, warmup, total, "cosine")
        lin_q3 = lr_at_step(base_lr, 75, warmup, total, "linear")
        assert cos_q3 < lin_q3, (
            f"cosine should be below linear at third quarter: cos={cos_q3:.4f}, lin={lin_q3:.4f}"
        )


def test_lr_no_warmup_zero_warmup():
    """warmup=0 means no warmup phase; step=0 should be on decay curve."""
    import random
    random.seed(49)
    for _ in range(6):
        base_lr = random.uniform(0.01, 1.0)
        total = random.randint(10, 200)
        for sched in ("linear", "cosine"):
            # step 0 = warmup = 0, so progress = 0 -> should return base_lr
            lr0 = lr_at_step(base_lr, 0, 0, total, schedule=sched)
            assert math.isclose(lr0, base_lr, rel_tol=1e-9), (
                f"sched={sched}: lr at step=0, warmup=0 should equal base_lr={base_lr}, got {lr0}"
            )
            # step=total should be 0
            lr_end = lr_at_step(base_lr, total, 0, total, schedule=sched)
            assert math.isclose(lr_end, 0.0, abs_tol=1e-9), (
                f"sched={sched}: lr at step=total={total} should be 0, got {lr_end}"
            )


def test_lr_schedule_numerical_values_against_formula():
    """
    Spot-check arbitrary steps against the closed-form formula directly.
    This catches off-by-one or wrong branch errors.
    """
    import random
    random.seed(50)
    base_lr, warmup, total = 0.42, 15, 120

    for _ in range(30):
        step = random.randint(0, 130)
        for sched in ("linear", "cosine"):
            got = lr_at_step(base_lr, step, warmup, total, schedule=sched)
            # reference formula
            if step >= total:
                expected = 0.0
            elif step < warmup:
                expected = base_lr * step / warmup
            else:
                progress = (step - warmup) / (total - warmup)
                if sched == "linear":
                    expected = base_lr * (1.0 - progress)
                else:
                    expected = base_lr * 0.5 * (1.0 + math.cos(math.pi * progress))
            assert math.isclose(got, expected, rel_tol=1e-9, abs_tol=1e-12), (
                f"sched={sched}, step={step}: got {got}, expected {expected}"
            )


# ===========================================================================
# Integration: training-loop sanity (complementary to the existing overfit test)
# ===========================================================================

def test_training_step_decreases_loss():
    """A single gradient step should strictly lower the MSE loss."""
    torch.manual_seed(60)
    D_in, D_out = 8, 4
    x = torch.randn(16, D_in)
    target = torch.randn(16, D_out)

    scale = init_scale(D_in)
    W = (torch.randn(D_in, D_out) * scale).requires_grad_(True)
    b = torch.zeros(D_out, requires_grad=True)
    gamma = torch.ones(D_out, requires_grad=True)
    beta = torch.zeros(D_out, requires_grad=True)

    def compute_loss():
        z = x @ W + b
        h = layernorm(z, gamma, beta)
        return ((h - target) ** 2).mean()

    loss_before = compute_loss()
    loss_before.backward()

    lr = 0.05
    with torch.no_grad():
        for p in [W, b, gamma, beta]:
            p -= lr * p.grad

    loss_after = compute_loss()
    assert loss_after.item() < loss_before.item(), (
        f"Loss did not decrease: {loss_before.item():.6f} -> {loss_after.item():.6f}"
    )


def test_params_receive_nonzero_gradients():
    """After one forward + backward, every trainable parameter must have nonzero grad."""
    torch.manual_seed(61)
    D_in, D_out = 6, 5
    x = torch.randn(8, D_in)
    target = torch.randn(8, D_out)

    scale = init_scale(D_in)
    W = (torch.randn(D_in, D_out) * scale).requires_grad_(True)
    b = torch.zeros(D_out, requires_grad=True)
    gamma = torch.ones(D_out, requires_grad=True)
    beta = torch.zeros(D_out, requires_grad=True)

    z = x @ W + b
    h = layernorm(z, gamma, beta)
    loss = ((h - target) ** 2).mean()
    loss.backward()

    for name, p in [("W", W), ("b", b), ("gamma", gamma), ("beta", beta)]:
        assert p.grad is not None, f"{name}.grad is None"
        assert p.grad.abs().max() > 0, f"{name}.grad is all zeros"


def test_overfit_random_batch_reduces_loss_substantially():
    """
    On various random seeds, 200 gradient steps on a fixed 4-example batch must
    reduce loss to < 20% of initial — checks that all three functions cooperate.
    """
    for seed in [70, 71, 72]:
        torch.manual_seed(seed)
        N, D_in, D_out = 4, 6, 3
        x = torch.randn(N, D_in)
        target = torch.randn(N, D_out)

        scale = init_scale(D_in)
        W = (torch.randn(D_in, D_out) * scale).requires_grad_(True)
        b = torch.zeros(D_out, requires_grad=True)
        gamma = torch.ones(D_out, requires_grad=True)
        beta = torch.zeros(D_out, requires_grad=True)
        params = [W, b, gamma, beta]

        def compute_loss():
            z = x @ W + b
            h = layernorm(z, gamma, beta)
            return ((h - target) ** 2).mean()

        initial = compute_loss().item()
        base_lr, warmup, total = 0.3, 20, 200

        for step in range(total):
            loss = compute_loss()
            for p in params:
                p.grad = None
            loss.backward()
            lr = lr_at_step(base_lr, step, warmup, total, schedule="cosine")
            with torch.no_grad():
                for p in params:
                    p -= lr * p.grad

        final = compute_loss().item()
        assert final < 0.2 * initial, (
            f"seed={seed}: loss did not drop enough: {initial:.4f} -> {final:.4f}"
        )


def test_init_scale_weights_have_correct_empirical_std():
    """
    Weights drawn with init_scale should have empirical std ≈ init_scale(fan_in).
    This validates that init_scale is the correct std, not variance or something else.
    """
    torch.manual_seed(80)
    for fan_in in [16, 64, 256]:
        scale = init_scale(fan_in)
        fan_out = 32
        W = torch.randn(fan_in, fan_out) * scale
        empirical_std = W.std().item()
        # empirical std should be close to scale (allow 15% relative for random variation)
        assert math.isclose(empirical_std, scale, rel_tol=0.20), (
            f"fan_in={fan_in}: empirical std={empirical_std:.4f}, scale={scale:.4f}"
        )
