"""Property / randomized tests for module 08 — scalar autograd (Value).

Design goals
------------
* Every test uses a deterministic seed (varied per test) so failures are reproducible.
* Gradient correctness is verified BOTH via a PyTorch oracle (torch.autograd) AND
  via independent finite-difference (central-difference) checks.
* Invariants (relu >= 0, tanh in (-1,1), grad accumulation, topology) are tested on
  multiple random inputs, not a single fixed example.
* Edge cases: zero inputs, negative inputs, large magnitudes, single-node graph, deep chains.

Import contract
---------------
All calls to micrograd happen *inside* test functions so that this file can be imported
against an unfilled stub (raises NotImplementedError) without failing at collect time.
"""
from __future__ import annotations

import math

import numpy as np
import pytest
import torch

from micrograd import MLP, Layer, Neuron, Value


# ---------------------------------------------------------------------------
# Helpers
# ---------------------------------------------------------------------------

def central_diff(f, x: float, h: float = 1e-5) -> float:
    """Central-difference numerical derivative of scalar->scalar f at x."""
    return (f(x + h) - f(x - h)) / (2.0 * h)


def build_and_backward(expr_fn, *inputs):
    """
    Given a function that takes Value objects and returns a Value,
    run forward + backward and return the output Value.
    """
    vals = [Value(x) for x in inputs]
    out = expr_fn(*vals)
    out.backward()
    return out, vals


# ---------------------------------------------------------------------------
# 1. Forward values match PyTorch — tanh
# ---------------------------------------------------------------------------

def test_tanh_forward_matches_torch_random():
    """Value.tanh() forward must match torch.tanh on random inputs."""
    np.random.seed(1)
    xs = np.random.uniform(-3.0, 3.0, 30)
    for x in xs:
        got = Value(x).tanh().data
        want = float(torch.tanh(torch.tensor(x, dtype=torch.float64)))
        assert math.isclose(got, want, rel_tol=1e-9, abs_tol=1e-12), (
            f"tanh({x}): got {got}, want {want}"
        )


def test_tanh_output_in_valid_range():
    """tanh output must be in [-1, 1] for all finite inputs."""
    np.random.seed(2)
    xs = np.concatenate([
        np.random.uniform(-10.0, 10.0, 40),
        np.array([0.0, 1e-12, -1e-12, 50.0, -50.0]),
    ])
    for x in xs:
        t = Value(x).tanh().data
        # float64 tanh can saturate to exactly ±1 for |x| >= ~19; that is correct.
        assert -1.0 <= t <= 1.0, f"tanh({x}) = {t} is outside [-1, 1]"
        # For moderate inputs the output must be strictly between -1 and 1
        if abs(x) < 18.0:
            assert -1.0 < t < 1.0, f"tanh({x}) = {t} should be strictly inside (-1, 1)"


# ---------------------------------------------------------------------------
# 2. Forward values match PyTorch — relu
# ---------------------------------------------------------------------------

def test_relu_forward_matches_torch_random():
    """Value.relu() forward must match torch.relu on random inputs."""
    np.random.seed(3)
    xs = np.random.uniform(-5.0, 5.0, 30)
    for x in xs:
        got = Value(x).relu().data
        want = float(torch.relu(torch.tensor(x, dtype=torch.float64)))
        assert math.isclose(got, want, rel_tol=1e-9, abs_tol=1e-12), (
            f"relu({x}): got {got}, want {want}"
        )


def test_relu_output_nonneg():
    """relu output must always be >= 0."""
    np.random.seed(4)
    xs = np.random.uniform(-20.0, 20.0, 50)
    for x in xs:
        r = Value(x).relu().data
        assert r >= 0.0, f"relu({x}) = {r} is negative"


def test_relu_identity_for_positive():
    """relu(x) == x for x > 0."""
    np.random.seed(5)
    xs = np.random.uniform(0.01, 10.0, 30)
    for x in xs:
        r = Value(x).relu().data
        assert math.isclose(r, x, rel_tol=1e-12), f"relu({x}) != {x}, got {r}"


def test_relu_zero_for_negative():
    """relu(x) == 0 for x < 0."""
    np.random.seed(6)
    xs = np.random.uniform(-10.0, -0.01, 30)
    for x in xs:
        r = Value(x).relu().data
        assert r == 0.0, f"relu({x}) = {r}, expected 0.0"


# ---------------------------------------------------------------------------
# 3. Gradient vs PyTorch oracle — tanh
# ---------------------------------------------------------------------------

def test_tanh_grad_matches_torch_oracle():
    """
    d/dx tanh(x) computed via Value.backward() must match torch.autograd.
    Test on random inputs including near-saturation (large |x|).
    """
    np.random.seed(7)
    xs = np.concatenate([
        np.random.uniform(-3.0, 3.0, 20),
        np.array([5.0, -5.0, 0.0, 0.1, -0.1]),
    ])
    for x in xs:
        # micrograd
        v = Value(x)
        out = v.tanh()
        out.backward()
        mg_grad = v.grad

        # torch oracle
        t = torch.tensor(x, dtype=torch.float64, requires_grad=True)
        tout = torch.tanh(t)
        tout.backward()
        torch_grad = float(t.grad)

        assert math.isclose(mg_grad, torch_grad, rel_tol=1e-9, abs_tol=1e-12), (
            f"tanh grad at x={x}: micrograd={mg_grad}, torch={torch_grad}"
        )


# ---------------------------------------------------------------------------
# 4. Gradient vs PyTorch oracle — relu
# ---------------------------------------------------------------------------

def test_relu_grad_matches_torch_oracle():
    """
    d/dx relu(x) via Value.backward() must match torch.autograd.
    Skip x==0 (subdifferential point; both may return 0 but convention varies).
    """
    np.random.seed(8)
    xs = np.concatenate([
        np.random.uniform(-5.0, -0.01, 15),
        np.random.uniform(0.01, 5.0, 15),
    ])
    for x in xs:
        v = Value(x)
        out = v.relu()
        out.backward()
        mg_grad = v.grad

        t = torch.tensor(x, dtype=torch.float64, requires_grad=True)
        tout = torch.relu(t)
        tout.backward()
        torch_grad = float(t.grad)

        assert math.isclose(mg_grad, torch_grad, rel_tol=1e-9, abs_tol=1e-12), (
            f"relu grad at x={x}: micrograd={mg_grad}, torch={torch_grad}"
        )


# ---------------------------------------------------------------------------
# 5. Gradient vs PyTorch oracle — composed expressions (a*b + c).tanh()
# ---------------------------------------------------------------------------

def test_composed_grad_matches_torch_oracle():
    """
    For out = tanh(a*b + c), gradients w.r.t. a, b, c must match torch autograd
    on multiple random input triples.
    """
    np.random.seed(9)
    triples = np.random.uniform(-2.0, 2.0, (15, 3))
    for a0, b0, c0 in triples:
        # micrograd
        a = Value(a0); b = Value(b0); c = Value(c0)
        out = (a * b + c).tanh()
        out.backward()

        # torch oracle
        ta = torch.tensor(a0, dtype=torch.float64, requires_grad=True)
        tb = torch.tensor(b0, dtype=torch.float64, requires_grad=True)
        tc = torch.tensor(c0, dtype=torch.float64, requires_grad=True)
        tout = torch.tanh(ta * tb + tc)
        tout.backward()

        assert math.isclose(a.grad, float(ta.grad), abs_tol=1e-9), \
            f"a.grad mismatch at ({a0},{b0},{c0}): {a.grad} vs {float(ta.grad)}"
        assert math.isclose(b.grad, float(tb.grad), abs_tol=1e-9), \
            f"b.grad mismatch at ({a0},{b0},{c0}): {b.grad} vs {float(tb.grad)}"
        assert math.isclose(c.grad, float(tc.grad), abs_tol=1e-9), \
            f"c.grad mismatch at ({a0},{b0},{c0}): {c.grad} vs {float(tc.grad)}"


def test_composed_relu_relu_grad_matches_torch_oracle():
    """
    For out = relu(a*b) + relu(c), gradients must match torch autograd.
    """
    np.random.seed(10)
    triples = np.random.uniform(-3.0, 3.0, (20, 3))
    for a0, b0, c0 in triples:
        # skip degenerate zero inputs (subdifferential)
        if abs(a0 * b0) < 0.05 or abs(c0) < 0.05:
            continue

        # micrograd
        a = Value(a0); b = Value(b0); c = Value(c0)
        out = (a * b).relu() + c.relu()
        out.backward()

        # torch oracle
        ta = torch.tensor(a0, dtype=torch.float64, requires_grad=True)
        tb = torch.tensor(b0, dtype=torch.float64, requires_grad=True)
        tc = torch.tensor(c0, dtype=torch.float64, requires_grad=True)
        tout = torch.relu(ta * tb) + torch.relu(tc)
        tout.backward()

        assert math.isclose(a.grad, float(ta.grad), abs_tol=1e-9), \
            f"a.grad mismatch: {a.grad} vs {float(ta.grad)}"
        assert math.isclose(b.grad, float(tb.grad), abs_tol=1e-9), \
            f"b.grad mismatch: {b.grad} vs {float(tb.grad)}"
        assert math.isclose(c.grad, float(tc.grad), abs_tol=1e-9), \
            f"c.grad mismatch: {c.grad} vs {float(tc.grad)}"


# ---------------------------------------------------------------------------
# 6. Finite-difference gradient checks
# ---------------------------------------------------------------------------

def test_finite_diff_tanh_chain():
    """Central-difference check for a chain: tanh(tanh(x))."""
    np.random.seed(11)
    xs = np.random.uniform(-2.0, 2.0, 20)
    for x0 in xs:
        v = Value(x0)
        out = v.tanh().tanh()
        out.backward()
        mg_grad = v.grad

        fd_grad = central_diff(lambda x: math.tanh(math.tanh(x)), x0)
        assert math.isclose(mg_grad, fd_grad, rel_tol=1e-6, abs_tol=1e-8), (
            f"tanh(tanh(x)) grad at {x0}: micrograd={mg_grad}, fd={fd_grad}"
        )


def test_finite_diff_add_mul():
    """Finite-difference check: out = a*(b + a) for random (a, b)."""
    np.random.seed(12)
    pairs = np.random.uniform(-3.0, 3.0, (20, 2))
    for a0, b0 in pairs:
        # da
        a = Value(a0); b = Value(b0)
        out = a * (b + a)
        out.backward()
        mg_da = a.grad

        fd_da = central_diff(lambda x: x * (b0 + x), a0)
        assert math.isclose(mg_da, fd_da, rel_tol=1e-6, abs_tol=1e-8), (
            f"a*(b+a) da at ({a0},{b0}): micrograd={mg_da}, fd={fd_da}"
        )

        # db
        a = Value(a0); b = Value(b0)
        out = a * (b + a)
        out.backward()
        mg_db = b.grad

        fd_db = central_diff(lambda x: a0 * (x + a0), b0)
        assert math.isclose(mg_db, fd_db, rel_tol=1e-6, abs_tol=1e-8), (
            f"a*(b+a) db at ({a0},{b0}): micrograd={mg_db}, fd={fd_db}"
        )


def test_finite_diff_deep_mixed_chain():
    """
    Finite-difference on a deeper expression:
    out = tanh(relu(a)*b + tanh(c)) for random (a,b,c).
    Skips points where relu(a) is near 0 (kink).
    """
    np.random.seed(13)
    triples = np.random.uniform(-2.0, 2.0, (25, 3))
    h = 1e-5
    for a0, b0, c0 in triples:
        if abs(a0) < 0.1:
            continue  # near relu kink: skip

        def fval(av, bv, cv):
            a, b, c = Value(av), Value(bv), Value(cv)
            return (a.relu() * b + c.tanh()).tanh().data

        a = Value(a0); b = Value(b0); c = Value(c0)
        out = (a.relu() * b + c.tanh()).tanh()
        out.backward()

        fd_da = (fval(a0 + h, b0, c0) - fval(a0 - h, b0, c0)) / (2 * h)
        fd_db = (fval(a0, b0 + h, c0) - fval(a0, b0 - h, c0)) / (2 * h)
        fd_dc = (fval(a0, b0, c0 + h) - fval(a0, b0, c0 - h)) / (2 * h)

        assert math.isclose(a.grad, fd_da, rel_tol=1e-5, abs_tol=1e-7), \
            f"a.grad at ({a0},{b0},{c0}): {a.grad} vs fd {fd_da}"
        assert math.isclose(b.grad, fd_db, rel_tol=1e-5, abs_tol=1e-7), \
            f"b.grad at ({a0},{b0},{c0}): {b.grad} vs fd {fd_db}"
        assert math.isclose(c.grad, fd_dc, rel_tol=1e-5, abs_tol=1e-7), \
            f"c.grad at ({a0},{b0},{c0}): {c.grad} vs fd {fd_dc}"


# ---------------------------------------------------------------------------
# 7. Gradient accumulation — multi-use nodes
# ---------------------------------------------------------------------------

def test_grad_accumulation_cubic():
    """x^3 via repeated multiplication: grad should be 3x^2."""
    np.random.seed(14)
    xs = np.random.uniform(-3.0, 3.0, 20)
    for x0 in xs:
        x = Value(x0)
        y = x * x * x  # x^3
        y.backward()
        expected = 3.0 * x0 ** 2
        assert math.isclose(x.grad, expected, rel_tol=1e-9, abs_tol=1e-12), (
            f"x^3 grad at x={x0}: got {x.grad}, expected {expected}"
        )


def test_grad_accumulation_triple_use():
    """f = x*x + x — node used three times; grad = 2x + 1."""
    np.random.seed(15)
    xs = np.random.uniform(-5.0, 5.0, 20)
    for x0 in xs:
        x = Value(x0)
        y = x * x + x
        y.backward()
        expected = 2.0 * x0 + 1.0
        assert math.isclose(x.grad, expected, rel_tol=1e-9, abs_tol=1e-12), (
            f"x^2+x grad at x={x0}: got {x.grad}, expected {expected}"
        )


def test_grad_accumulation_matches_torch_multiuse():
    """Torch oracle comparison for a node used multiple times."""
    np.random.seed(16)
    xs = np.random.uniform(-3.0, 3.0, 20)
    for x0 in xs:
        # micrograd: f = tanh(x) * x + x
        x = Value(x0)
        y = x.tanh() * x + x
        y.backward()
        mg_grad = x.grad

        # torch oracle
        t = torch.tensor(x0, dtype=torch.float64, requires_grad=True)
        ty = torch.tanh(t) * t + t
        ty.backward()
        torch_grad = float(t.grad)

        assert math.isclose(mg_grad, torch_grad, abs_tol=1e-9), (
            f"tanh(x)*x+x grad at x={x0}: micrograd={mg_grad}, torch={torch_grad}"
        )


# ---------------------------------------------------------------------------
# 8. Topology: .grad is 0.0 before backward
# ---------------------------------------------------------------------------

def test_grad_zero_before_backward_random():
    """All nodes must have .grad == 0.0 before backward is called."""
    np.random.seed(17)
    vals = np.random.uniform(-5.0, 5.0, 8)
    for a0, b0, c0, d0 in vals.reshape(-1, 4):
        a = Value(a0); b = Value(b0); c = Value(c0); d = Value(d0)
        out = (a * b + c).tanh() + d.relu()
        for node in [a, b, c, d, out]:
            assert node.grad == 0.0, f"grad should be 0 before backward, got {node.grad}"


# ---------------------------------------------------------------------------
# 9. Backward sets loss.grad = 1.0
# ---------------------------------------------------------------------------

def test_backward_sets_root_grad_to_one():
    """After backward(), the node backward is called from must have .grad == 1.0."""
    np.random.seed(18)
    xs = np.random.uniform(-3.0, 3.0, 15)
    for x0 in xs:
        v = Value(x0)
        out = v.tanh()
        out.backward()
        assert out.grad == 1.0, f"root.grad after backward should be 1.0, got {out.grad}"


# ---------------------------------------------------------------------------
# 10. Large-magnitude stability (tanh saturation)
# ---------------------------------------------------------------------------

def test_tanh_large_magnitude_stability():
    """tanh should not produce NaN or Inf for large |x|; grad approaches 0."""
    np.random.seed(19)
    xs = [100.0, -100.0, 1e6, -1e6, 50.0, -50.0]
    for x0 in xs:
        v = Value(x0)
        out = v.tanh()
        out.backward()
        assert math.isfinite(out.data), f"tanh({x0}).data is not finite: {out.data}"
        assert math.isfinite(v.grad), f"tanh({x0}) grad is not finite: {v.grad}"
        # for large |x|, tanh saturates and grad should be near 0
        assert abs(v.grad) < 1e-6, f"tanh({x0}) grad should be ~0, got {v.grad}"


def test_relu_large_magnitude():
    """relu and its gradient should be numerically stable for large inputs."""
    np.random.seed(20)
    xs = [1e6, -1e6, 1e9, -1e9]
    for x0 in xs:
        v = Value(x0)
        out = v.relu()
        out.backward()
        assert math.isfinite(out.data), f"relu({x0}).data is not finite"
        assert math.isfinite(v.grad), f"relu({x0}) grad is not finite"


# ---------------------------------------------------------------------------
# 11. Edge cases: zero inputs
# ---------------------------------------------------------------------------

def test_zero_input_forward_and_backward():
    """All ops at x=0 must produce correct forward values and gradients."""
    # tanh(0) = 0, grad = 1
    v = Value(0.0)
    out = v.tanh()
    out.backward()
    assert out.data == 0.0
    assert math.isclose(v.grad, 1.0, rel_tol=1e-12)

    # relu(0) = 0, grad = 0 (convention: 0 at kink)
    v = Value(0.0)
    out = v.relu()
    out.backward()
    assert out.data == 0.0
    assert v.grad == 0.0

    # add(0, 0) = 0
    a = Value(0.0); b = Value(0.0)
    out = a + b
    out.backward()
    assert out.data == 0.0
    assert a.grad == 1.0 and b.grad == 1.0

    # mul(0, 5) = 0, grad_a = 5, grad_b = 0
    a = Value(0.0); b = Value(5.0)
    out = a * b
    out.backward()
    assert out.data == 0.0
    assert math.isclose(a.grad, 5.0, rel_tol=1e-12)
    assert math.isclose(b.grad, 0.0, abs_tol=1e-12)


# ---------------------------------------------------------------------------
# 12. Single-node graph: backward on a leaf
# ---------------------------------------------------------------------------

def test_backward_on_leaf_node():
    """Calling backward on a leaf Value sets its own .grad to 1.0."""
    v = Value(42.0)
    v.backward()
    assert v.grad == 1.0, f"leaf.grad after backward should be 1.0, got {v.grad}"


# ---------------------------------------------------------------------------
# 13. Subtraction and negation gradient correctness (via oracle)
# ---------------------------------------------------------------------------

def test_sub_and_neg_grad_matches_torch():
    """Gradients for subtraction and negation must match torch autograd."""
    np.random.seed(21)
    pairs = np.random.uniform(-3.0, 3.0, (15, 2))
    for a0, b0 in pairs:
        # f = a - b
        a = Value(a0); b = Value(b0)
        out = a - b
        out.backward()

        ta = torch.tensor(a0, dtype=torch.float64, requires_grad=True)
        tb = torch.tensor(b0, dtype=torch.float64, requires_grad=True)
        tout = ta - tb
        tout.backward()

        assert math.isclose(a.grad, float(ta.grad), abs_tol=1e-12), \
            f"(a-b) a.grad: {a.grad} vs {float(ta.grad)}"
        assert math.isclose(b.grad, float(tb.grad), abs_tol=1e-12), \
            f"(a-b) b.grad: {b.grad} vs {float(tb.grad)}"

        # f = -a + tanh(b)
        a = Value(a0); b = Value(b0)
        out = (-a) + b.tanh()
        out.backward()

        ta = torch.tensor(a0, dtype=torch.float64, requires_grad=True)
        tb = torch.tensor(b0, dtype=torch.float64, requires_grad=True)
        tout = (-ta) + torch.tanh(tb)
        tout.backward()

        assert math.isclose(a.grad, float(ta.grad), abs_tol=1e-12), \
            f"(-a+tanh(b)) a.grad: {a.grad} vs {float(ta.grad)}"
        assert math.isclose(b.grad, float(tb.grad), abs_tol=1e-9), \
            f"(-a+tanh(b)) b.grad: {b.grad} vs {float(tb.grad)}"


# ---------------------------------------------------------------------------
# 14. Long chain — gradient vanishing via tanh saturation
# ---------------------------------------------------------------------------

def test_long_tanh_chain_grad_vanishes():
    """
    For a chain of N tanh activations with a large initial value, the gradient
    at the leaf should be extremely small (demonstrating vanishing gradients).
    """
    np.random.seed(22)
    # Start from a moderately large value; each tanh saturates further
    x = Value(3.0)
    out = x
    depth = 10
    for _ in range(depth):
        out = out.tanh()
    out.backward()
    # The gradient must be valid (not NaN/Inf) and should be very small
    assert math.isfinite(x.grad), f"long chain grad not finite: {x.grad}"
    assert x.grad >= 0.0, f"long chain grad should be non-negative: {x.grad}"
    assert x.grad < 1e-3, f"expected vanishing gradient, got {x.grad}"


# ---------------------------------------------------------------------------
# 15. Mixed deep expression — oracle check with torch, multiple random seeds
# ---------------------------------------------------------------------------

def test_deep_mixed_expression_oracle():
    """
    out = tanh(relu(a) * b + tanh(a + c)) + b * c
    Gradients w.r.t. a, b, c verified against torch autograd on random triples.
    """
    np.random.seed(23)
    torch.manual_seed(23)
    triples = np.random.uniform(-2.0, 2.0, (20, 3))
    for a0, b0, c0 in triples:
        if abs(a0) < 0.1:
            continue  # skip relu kink

        # micrograd
        a = Value(a0); b = Value(b0); c = Value(c0)
        out = (a.relu() * b + (a + c).tanh()).tanh() + b * c
        out.backward()

        # torch oracle
        ta = torch.tensor(a0, dtype=torch.float64, requires_grad=True)
        tb = torch.tensor(b0, dtype=torch.float64, requires_grad=True)
        tc = torch.tensor(c0, dtype=torch.float64, requires_grad=True)
        tout = torch.tanh(torch.relu(ta) * tb + torch.tanh(ta + tc)) + tb * tc
        tout.backward()

        assert math.isclose(a.grad, float(ta.grad), abs_tol=1e-9), \
            f"deep a.grad at ({a0},{b0},{c0}): {a.grad} vs {float(ta.grad)}"
        assert math.isclose(b.grad, float(tb.grad), abs_tol=1e-9), \
            f"deep b.grad at ({a0},{b0},{c0}): {b.grad} vs {float(tb.grad)}"
        assert math.isclose(c.grad, float(tc.grad), abs_tol=1e-9), \
            f"deep c.grad at ({a0},{b0},{c0}): {c.grad} vs {float(tc.grad)}"


# ---------------------------------------------------------------------------
# 16. Task 7 — Neuron / Layer / MLP built on top of Value
#
# These tests cover the "top of the micrograd lecture": a real network assembled
# from Value nodes. We verify (a) forward matches a hand-written numpy/torch
# reference, (b) backward propagates correct gradients to EVERY weight (vs
# finite-difference), and (c) SGD on a toy XOR-like set actually reduces loss.
# ---------------------------------------------------------------------------

import random as _random  # noqa: E402  (kept local to this section)


def _set_mlp_params(net: MLP, values):
    """Overwrite every parameter .data of `net` from a flat iterable `values`."""
    ps = net.parameters()
    assert len(values) == len(ps)
    for p, v in zip(ps, values):
        p.data = float(v)


def _torch_mlp_forward(net: MLP, x):
    """
    Recompute the forward pass of `net` in pure torch, reading weights/biases
    straight off the Value parameters. Layout per neuron: [w_0..w_{n-1}, b].
    Hidden layers use tanh, the last layer is linear (matching MLP's default).
    """
    out = torch.tensor([float(xi) for xi in x], dtype=torch.float64)
    n_layers = len(net.layers)
    for li, layer in enumerate(net.layers):
        neuron_outs = []
        for neuron in layer.neurons:
            w = torch.tensor([wi.data for wi in neuron.w], dtype=torch.float64)
            b = torch.tensor(neuron.b.data, dtype=torch.float64)
            act = torch.dot(w, out) + b
            if neuron.nonlin == "tanh":
                act = torch.tanh(act)
            elif neuron.nonlin == "relu":
                act = torch.relu(act)
            # 'linear' -> leave as is
            neuron_outs.append(act)
        out = torch.stack(neuron_outs)
    return out


def test_mlp_forward_matches_torch_oracle():
    """
    MLP([3, 4, 4, 1]) forward output (a single Value) must match a torch
    recomputation of the same weights to 1e-6, on several random inputs.
    """
    rng = _random.Random(101)
    net = MLP([3, 4, 4, 1], rng=rng)

    np.random.seed(101)
    for _ in range(10):
        x = np.random.uniform(-2.0, 2.0, 3)
        got = net(list(x))
        assert isinstance(got, Value), "single-output MLP must return a lone Value"

        want = _torch_mlp_forward(net, x)
        assert want.shape == (1,)
        assert math.isclose(got.data, float(want[0]), rel_tol=1e-9, abs_tol=1e-6), (
            f"MLP forward {got.data} vs torch {float(want[0])} at x={x}"
        )


def test_neuron_and_layer_shapes_and_params():
    """Structural invariants: parameter counts and Layer output cardinality."""
    rng = _random.Random(202)

    # Neuron(n_in) has n_in weights + 1 bias.
    n = Neuron(5, nonlin="tanh", rng=rng)
    assert len(n.parameters()) == 5 + 1
    assert isinstance(n([0.0] * 5), Value)

    # Layer(n_in, n_out) returns n_out Values and owns n_out*(n_in+1) params.
    layer = Layer(3, 4, nonlin="tanh", rng=rng)
    ys = layer([1.0, 2.0, 3.0])
    assert isinstance(ys, list) and len(ys) == 4
    assert all(isinstance(y, Value) for y in ys)
    assert len(layer.parameters()) == 4 * (3 + 1)

    # MLP([2,3,1]) -> hidden 3 neurons (each 2w+1b) + output 1 neuron (3w+1b).
    net = MLP([2, 3, 1], rng=rng)
    assert len(net.parameters()) == 3 * (2 + 1) + 1 * (3 + 1)


def test_mlp_backward_matches_finite_difference():
    """
    For MLP(2,3,1) at a fixed random input, backward() must fill .grad of EVERY
    weight to match a central finite-difference estimate (atol=1e-5). This is the
    real "does autograd reach all weights" check, not just the output node.
    """
    rng = _random.Random(303)
    net = MLP([2, 3, 1], rng=rng)
    params = net.parameters()
    x = [0.7, -1.3]

    # Backward from the scalar output.
    net.zero_grad()
    out = net(x)
    out.backward()
    analytic = [p.grad for p in params]

    # Finite difference: perturb each parameter's .data in place.
    h = 1e-5
    for i, p in enumerate(params):
        orig = p.data

        p.data = orig + h
        f_plus = net(x).data
        p.data = orig - h
        f_minus = net(x).data
        p.data = orig  # restore

        fd = (f_plus - f_minus) / (2 * h)
        assert math.isclose(analytic[i], fd, rel_tol=1e-5, abs_tol=1e-5), (
            f"param[{i}] grad {analytic[i]} vs finite-diff {fd}"
        )


def test_mlp_sgd_reduces_xor_loss_monotonically():
    """
    Train MLP([2, 8, 8, 1]) on the XOR toy set with manual SGD.
    Invariants:
      * loss decreases at 3 checkpoints (catches zero_grad / sign bugs);
      * final loss is at most 50% of the initial loss.
    XOR is the canonical "needs a hidden layer" task — a single neuron cannot fit it.
    """
    rng = _random.Random(404)
    net = MLP([2, 8, 8, 1], rng=rng)

    # XOR: label +1 when inputs differ, -1 when they match (tanh-friendly targets).
    xs = [[0.0, 0.0], [0.0, 1.0], [1.0, 0.0], [1.0, 1.0]]
    ys = [-1.0, 1.0, 1.0, -1.0]

    def mean_loss() -> float:
        total = Value(0.0)
        for x, y in zip(xs, ys):
            pred = net(x)
            diff = pred - y
            total = total + diff * diff
        return (total * (1.0 / len(xs))).data

    loss0 = mean_loss()
    checkpoints = []
    lr = 0.05
    steps = 50
    for step in range(steps):
        # forward — accumulate squared error over the 4 examples
        total = Value(0.0)
        for x, y in zip(xs, ys):
            pred = net(x)
            diff = pred - y
            total = total + diff * diff
        loss = total * (1.0 / len(xs))

        net.zero_grad()        # <-- the optimizer.zero_grad() analogue
        loss.backward()
        for p in net.parameters():
            p.data -= lr * p.grad

        if step in (steps // 3, 2 * steps // 3, steps - 1):
            checkpoints.append(mean_loss())

    loss_final = checkpoints[-1]

    # Monotonic decrease across the 3 checkpoints.
    assert checkpoints[0] > checkpoints[1] > checkpoints[2], (
        f"loss must decrease monotonically at checkpoints, got {checkpoints}"
    )
    # Substantial overall reduction.
    assert loss_final < 0.5 * loss0, (
        f"final loss {loss_final} should be < 50% of initial {loss0}"
    )


def test_mlp_zero_grad_is_required():
    """
    Without zero_grad, gradients accumulate across steps. This test pins the
    contract: zero_grad() must reset every parameter's .grad to exactly 0.0,
    and a fresh backward then equals a single-step gradient.
    """
    rng = _random.Random(505)
    net = MLP([2, 3, 1], rng=rng)
    x = [0.4, -0.9]

    # First backward.
    net.zero_grad()
    net(x).backward()
    single = [p.grad for p in net.parameters()]

    # Second backward WITHOUT zero_grad -> grads roughly double (same graph value).
    net(x).backward()
    doubled = [p.grad for p in net.parameters()]
    for s, d in zip(single, doubled):
        assert math.isclose(d, 2.0 * s, rel_tol=1e-9, abs_tol=1e-12)

    # zero_grad clears everything back to 0.0.
    net.zero_grad()
    assert all(p.grad == 0.0 for p in net.parameters())
