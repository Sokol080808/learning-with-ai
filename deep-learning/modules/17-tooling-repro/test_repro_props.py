# Randomized property tests for module 17 — Reproducibility & Engineering.
#
# Rules:
#   - Each test fixes its own seed (varied per test) for full determinism.
#   - All module calls are INSIDE test functions (never at collect time).
#   - No module-level solution calls — the file imports/collects cleanly against
#     an unfilled stub that raises NotImplementedError; only FAILS at runtime.
#   - Oracle: PyTorch / stdlib / direct formula — never the functions under test.

import os
import random

import numpy as np
import pytest
import torch
import torch.nn as nn

from repro import set_seed, count_parameters, save_model, load_model


# ---------------------------------------------------------------------------
# Tiny helpers — defined here so there are no top-level calls to the module.
# ---------------------------------------------------------------------------

def _make_linear(d_in, d_out, bias=True):
    return nn.Linear(d_in, d_out, bias=bias)


def _expected_params(model):
    """Oracle: count trainable params directly via PyTorch, bypassing the SUT."""
    return sum(p.numel() for p in model.parameters() if p.requires_grad)


# ---------------------------------------------------------------------------
# PROPERTY 1  set_seed — same seed ⟹ identical results across all three RNGs
# ---------------------------------------------------------------------------

@pytest.mark.parametrize("seed", [0, 1, 13, 42, 255, 65535, 2**31 - 1])
def test_set_seed_torch_determinism_many_seeds(seed):
    """same seed → identical torch sequence; different subsequent draws differ."""
    set_seed(seed)
    a = torch.randn(16)
    set_seed(seed)
    b = torch.randn(16)
    assert torch.allclose(a, b), f"seed={seed}: second pass differs from first"
    # Without re-seeding the next draw must be a continuation — not equal
    c = torch.randn(16)
    assert not torch.allclose(a, c), (
        f"seed={seed}: consecutive draws are identical — seed not advancing"
    )


@pytest.mark.parametrize("seed", [3, 17, 99, 200, 1024])
def test_set_seed_numpy_determinism_many_seeds(seed):
    set_seed(seed)
    a = np.random.rand(20)
    set_seed(seed)
    b = np.random.rand(20)
    assert np.allclose(a, b), f"seed={seed}: numpy pass 2 ≠ pass 1"
    c = np.random.rand(20)
    assert not np.allclose(a, c), (
        f"seed={seed}: numpy consecutive draws identical — seed not advancing"
    )


@pytest.mark.parametrize("seed", [5, 50, 500, 5000])
def test_set_seed_python_random_determinism_many_seeds(seed):
    set_seed(seed)
    a = [random.random() for _ in range(15)]
    set_seed(seed)
    b = [random.random() for _ in range(15)]
    assert a == b, f"seed={seed}: python random pass 2 ≠ pass 1"
    c = [random.random() for _ in range(15)]
    assert a != c, (
        f"seed={seed}: python random consecutive draws identical — seed not advancing"
    )


def test_set_seed_different_seeds_diverge_all_three():
    """Two distinct seeds must produce different outputs on all three RNGs."""
    seeds = [10, 20, 30, 40, 50]
    for s1, s2 in zip(seeds, seeds[1:]):
        set_seed(s1)
        t1 = torch.randn(8)
        n1 = np.random.rand(8)
        r1 = [random.random() for _ in range(8)]

        set_seed(s2)
        t2 = torch.randn(8)
        n2 = np.random.rand(8)
        r2 = [random.random() for _ in range(8)]

        assert not torch.allclose(t1, t2), f"seeds {s1},{s2}: torch outputs equal"
        assert not np.allclose(n1, n2), f"seeds {s1},{s2}: numpy outputs equal"
        assert r1 != r2, f"seeds {s1},{s2}: python random outputs equal"


def test_set_seed_independence_all_three_rngs():
    """A single set_seed call resets ALL three generators simultaneously.

    Strategy: draw from torch before re-seeding, then call set_seed once, then
    draw from all three.  The torch draws before and after must be identical.
    """
    SEED = 77
    set_seed(SEED)
    ref_torch = torch.randn(6)
    ref_numpy = np.random.rand(6)
    ref_python = [random.random() for _ in range(6)]

    # advance all three generators
    torch.randn(100)
    np.random.rand(100)
    [random.random() for _ in range(100)]

    # single reset
    set_seed(SEED)
    got_torch = torch.randn(6)
    got_numpy = np.random.rand(6)
    got_python = [random.random() for _ in range(6)]

    assert torch.allclose(ref_torch, got_torch), "torch not reset by set_seed"
    assert np.allclose(ref_numpy, got_numpy), "numpy not reset by set_seed"
    assert ref_python == got_python, "python random not reset by set_seed"


def test_set_seed_edge_seed_zero():
    """Seed 0 is a valid edge-case value; must be idempotent."""
    set_seed(0)
    a = torch.randn(4)
    set_seed(0)
    b = torch.randn(4)
    assert torch.allclose(a, b)


def test_set_seed_large_seed():
    """Very large seed (near int32 max) must work without error."""
    large = 2**31 - 1
    set_seed(large)
    a = torch.randn(4)
    set_seed(large)
    b = torch.randn(4)
    assert torch.allclose(a, b)


# ---------------------------------------------------------------------------
# PROPERTY 2  count_parameters — matches oracle, respects requires_grad
# ---------------------------------------------------------------------------

@pytest.mark.parametrize("d_in,d_out", [
    (1, 1), (2, 5), (10, 10), (128, 64), (256, 1),
])
def test_count_parameters_linear_matches_formula(d_in, d_out):
    """Linear(d_in, d_out) with bias → d_in*d_out + d_out trainable params."""
    torch.manual_seed(d_in * 31 + d_out)
    model = _make_linear(d_in, d_out)
    expected = d_in * d_out + d_out
    assert count_parameters(model) == expected, (
        f"Linear({d_in},{d_out}): got {count_parameters(model)}, expected {expected}"
    )


@pytest.mark.parametrize("d_in,d_out", [(4, 3), (16, 8), (32, 32)])
def test_count_parameters_no_bias_matches_formula(d_in, d_out):
    """Linear without bias → exactly d_in*d_out trainable params."""
    torch.manual_seed(d_in + d_out * 7)
    model = _make_linear(d_in, d_out, bias=False)
    expected = d_in * d_out
    assert count_parameters(model) == expected


def test_count_parameters_sequential_random_shapes():
    """Multi-layer sequential network: sum equals per-layer oracle."""
    torch.manual_seed(88)
    dims = [8, 16, 16, 4, 2]           # 4 linear layers
    layers = []
    for i in range(len(dims) - 1):
        layers.append(nn.Linear(dims[i], dims[i + 1]))
        layers.append(nn.ReLU())
    model = nn.Sequential(*layers)
    assert count_parameters(model) == _expected_params(model)


def test_count_parameters_fully_frozen_is_zero():
    """Freezing all parameters → count_parameters returns 0."""
    torch.manual_seed(101)
    model = _make_linear(5, 3)
    for p in model.parameters():
        p.requires_grad_(False)
    assert count_parameters(model) == 0


@pytest.mark.parametrize("freeze_bias", [True, False])
def test_count_parameters_partial_freeze(freeze_bias):
    """Partially frozen model: only trainable params counted."""
    torch.manual_seed(202 + int(freeze_bias))
    model = _make_linear(6, 4)          # weight 24 + bias 4 = 28
    if freeze_bias:
        model.bias.requires_grad_(False)
        expected = 6 * 4               # only weight
    else:
        model.weight.requires_grad_(False)
        expected = 4                   # only bias
    assert count_parameters(model) == expected


def test_count_parameters_returns_int_type():
    """Return type must be plain Python int, not a tensor or numpy scalar."""
    torch.manual_seed(303)
    result = count_parameters(_make_linear(3, 3))
    assert type(result) is int, f"expected int, got {type(result)}"


def test_count_parameters_single_neuron():
    """Edge case: single output neuron (1 weight + 1 bias = 2 params)."""
    torch.manual_seed(404)
    assert count_parameters(_make_linear(1, 1)) == 2


def test_count_parameters_deep_random_network():
    """Randomized deep network: oracle vs. SUT must agree."""
    torch.manual_seed(505)
    widths = [np.random.randint(4, 32) for _ in range(8)]
    layers = []
    for a, b in zip(widths, widths[1:]):
        layers.append(nn.Linear(a, b))
        layers.append(nn.ReLU())
    model = nn.Sequential(*layers)
    assert count_parameters(model) == _expected_params(model)


def test_count_parameters_mixed_freeze_random():
    """Randomly freeze half the layers and compare with oracle."""
    torch.manual_seed(606)
    model = nn.Sequential(
        nn.Linear(8, 8),
        nn.ReLU(),
        nn.Linear(8, 8),
        nn.ReLU(),
        nn.Linear(8, 4),
    )
    # freeze even-indexed Linear layers
    linear_layers = [m for m in model.modules() if isinstance(m, nn.Linear)]
    for i, layer in enumerate(linear_layers):
        if i % 2 == 0:
            for p in layer.parameters():
                p.requires_grad_(False)
    assert count_parameters(model) == _expected_params(model)


# ---------------------------------------------------------------------------
# PROPERTY 3 & 4  save_model / load_model — round-trip and numerical identity
# ---------------------------------------------------------------------------

def test_save_load_roundtrip_random_weights(tmp_path):
    """Random weights survive save→load round-trip (allclose, atol=0)."""
    torch.manual_seed(707)
    src = nn.Sequential(nn.Linear(8, 8), nn.ReLU(), nn.Linear(8, 4))
    path = str(tmp_path / "model707.pt")
    save_model(src, path)

    torch.manual_seed(808)
    dst = nn.Sequential(nn.Linear(8, 8), nn.ReLU(), nn.Linear(8, 4))
    # Confirm they differ before load
    assert not torch.allclose(
        list(src.parameters())[0], list(dst.parameters())[0]
    )
    load_model(dst, path)
    for (n_s, p_s), (n_d, p_d) in zip(
        src.state_dict().items(), dst.state_dict().items()
    ):
        assert n_s == n_d
        assert torch.allclose(p_s, p_d, atol=0), (
            f"param '{n_s}': not bit-exact after round-trip"
        )


@pytest.mark.parametrize("seed", [11, 22, 33, 44])
def test_save_load_output_identity(tmp_path, seed):
    """Loaded model produces bit-identical forward outputs on random inputs."""
    torch.manual_seed(seed)
    src = nn.Sequential(nn.Linear(4, 8), nn.Tanh(), nn.Linear(8, 2))
    src.eval()
    path = str(tmp_path / f"model_{seed}.pt")
    save_model(src, path)

    torch.manual_seed(seed + 1000)
    dst = nn.Sequential(nn.Linear(4, 8), nn.Tanh(), nn.Linear(8, 2))
    load_model(dst, path)
    dst.eval()

    torch.manual_seed(seed + 2000)
    x = torch.randn(10, 4)
    with torch.no_grad():
        out_src = src(x)
        out_dst = dst(x)
    assert torch.allclose(out_src, out_dst, atol=1e-7), (
        f"seed={seed}: loaded model output differs"
    )


def test_save_load_returns_same_object(tmp_path):
    """load_model must return the exact same Python object it received."""
    torch.manual_seed(1001)
    src = _make_linear(4, 3)
    path = str(tmp_path / "id_check.pt")
    save_model(src, path)

    torch.manual_seed(1002)
    dst = _make_linear(4, 3)
    returned = load_model(dst, path)
    assert returned is dst, "load_model must return the same object passed as 'model'"


def test_save_load_file_contains_state_dict(tmp_path):
    """The saved file must be loadable as a plain dict (state_dict, not model)."""
    torch.manual_seed(1111)
    model = _make_linear(5, 3)
    path = str(tmp_path / "sd.pt")
    save_model(model, path)

    loaded = torch.load(path, weights_only=True)
    assert isinstance(loaded, dict), "saved file should be a dict (state_dict)"
    assert "weight" in loaded and "bias" in loaded


def test_save_load_large_model_random_shapes(tmp_path):
    """Stress: wider random layers, verify all parameter tensors allclose."""
    torch.manual_seed(2222)
    dims = [64, 128, 64, 32, 16]
    layers = []
    for a, b in zip(dims, dims[1:]):
        layers.append(nn.Linear(a, b))
        layers.append(nn.ReLU())
    src = nn.Sequential(*layers)
    path = str(tmp_path / "large.pt")
    save_model(src, path)

    torch.manual_seed(3333)
    layers2 = []
    for a, b in zip(dims, dims[1:]):
        layers2.append(nn.Linear(a, b))
        layers2.append(nn.ReLU())
    dst = nn.Sequential(*layers2)
    load_model(dst, path)

    for (n_s, p_s), (n_d, p_d) in zip(
        src.state_dict().items(), dst.state_dict().items()
    ):
        assert torch.allclose(p_s, p_d, atol=0), f"param '{n_s}' mismatch"


def test_save_creates_nonempty_file(tmp_path):
    """Saved file must exist and be non-empty."""
    torch.manual_seed(4444)
    model = _make_linear(10, 5)
    path = str(tmp_path / "nonempty.pt")
    save_model(model, path)
    assert os.path.exists(path)
    assert os.path.getsize(path) > 0


def test_save_load_eval_mode_preserved(tmp_path):
    """load_model does not silently switch training mode."""
    torch.manual_seed(5555)
    src = nn.Sequential(nn.Linear(4, 4), nn.Dropout(0.5))
    src.eval()
    path = str(tmp_path / "eval.pt")
    save_model(src, path)

    torch.manual_seed(6666)
    dst = nn.Sequential(nn.Linear(4, 4), nn.Dropout(0.5))
    dst.eval()                          # caller sets mode — load_model must not reset it
    load_model(dst, path)
    assert not dst.training, (
        "load_model must not force model into train mode"
    )


def test_save_load_single_row_input(tmp_path):
    """Edge case: batch size = 1 (single example)."""
    torch.manual_seed(7777)
    src = nn.Linear(3, 2)
    src.eval()
    path = str(tmp_path / "single.pt")
    save_model(src, path)

    dst = nn.Linear(3, 2)
    load_model(dst, path)
    dst.eval()

    x = torch.tensor([[1.0, -2.0, 0.5]])
    with torch.no_grad():
        assert torch.allclose(src(x), dst(x), atol=1e-7)


def test_save_load_zero_weights(tmp_path):
    """Edge case: manually zero all weights; round-trip must preserve zeros."""
    torch.manual_seed(8888)
    model = _make_linear(4, 3)
    with torch.no_grad():
        for p in model.parameters():
            p.zero_()
    path = str(tmp_path / "zeros.pt")
    save_model(model, path)

    dst = _make_linear(4, 3)
    load_model(dst, path)
    for p in dst.parameters():
        assert torch.all(p == 0), "zero weights not preserved after round-trip"


def test_save_load_negative_large_weights(tmp_path):
    """Edge case: large-magnitude weights (numerical stability check)."""
    torch.manual_seed(9999)
    model = _make_linear(4, 3)
    with torch.no_grad():
        model.weight.fill_(1e6)
        model.bias.fill_(-1e6)
    path = str(tmp_path / "large_mag.pt")
    save_model(model, path)

    dst = _make_linear(4, 3)
    load_model(dst, path)
    assert torch.allclose(model.weight, dst.weight, atol=0)
    assert torch.allclose(model.bias, dst.bias, atol=0)


# ---------------------------------------------------------------------------
# CROSS-PROPERTY: set_seed + save/load interaction
# ---------------------------------------------------------------------------

def test_set_seed_then_save_load_deterministic(tmp_path):
    """set_seed followed by model creation + save/load gives identical weights
    across two independent runs of the same sequence."""
    SEED = 31415

    set_seed(SEED)
    m1 = nn.Sequential(nn.Linear(6, 6), nn.ReLU(), nn.Linear(6, 3))
    path1 = str(tmp_path / "run1.pt")
    save_model(m1, path1)

    set_seed(SEED)
    m2 = nn.Sequential(nn.Linear(6, 6), nn.ReLU(), nn.Linear(6, 3))
    path2 = str(tmp_path / "run2.pt")
    save_model(m2, path2)

    # Both files must encode the same weights
    sd1 = torch.load(path1, weights_only=True)
    sd2 = torch.load(path2, weights_only=True)
    for key in sd1:
        assert torch.allclose(sd1[key], sd2[key], atol=0), (
            f"key '{key}': two identical-seed runs differ"
        )


def test_count_params_consistent_after_load(tmp_path):
    """count_parameters must return the same value before and after load_model."""
    torch.manual_seed(11111)
    src = nn.Sequential(nn.Linear(8, 4), nn.ReLU(), nn.Linear(4, 2))
    expected = count_parameters(src)
    path = str(tmp_path / "cp.pt")
    save_model(src, path)

    torch.manual_seed(22222)
    dst = nn.Sequential(nn.Linear(8, 4), nn.ReLU(), nn.Linear(4, 2))
    load_model(dst, path)
    assert count_parameters(dst) == expected
