# Интеграционные тесты для модуля 17 — Задание 5 (set_mode + run_reproducible_experiment).
#
# Rules:
#   - All calls to the new functions are INSIDE test functions — never at module level.
#     This means the file collects cleanly against a NotImplementedError stub on main.
#   - Oracles: torch / direct computation — never the functions under test.
#   - Seeds are fixed per test for full determinism.

import pytest
import torch
import torch.nn as nn

from repro import set_seed, count_parameters, set_mode, run_reproducible_experiment


# ---------------------------------------------------------------------------
# Helpers
# ---------------------------------------------------------------------------

def _tiny_model():
    """Two-layer network with Dropout so train/eval behaves differently."""
    return nn.Sequential(
        nn.Linear(4, 8),
        nn.Dropout(p=0.5),
        nn.ReLU(),
        nn.Linear(8, 2),
    )


# ---------------------------------------------------------------------------
# set_mode: basic contract
# ---------------------------------------------------------------------------

def test_set_mode_train_sets_training_true():
    model = _tiny_model()
    model.eval()  # start in eval
    result = set_mode(model, True)
    assert model.training is True, "set_mode(model, True) must put model in train mode"


def test_set_mode_eval_sets_training_false():
    model = _tiny_model()
    model.train()  # start in train
    result = set_mode(model, False)
    assert model.training is False, "set_mode(model, False) must put model in eval mode"


def test_set_mode_returns_same_object():
    model = _tiny_model()
    returned = set_mode(model, True)
    assert returned is model, "set_mode must return the same model object"


def test_set_mode_train_false_returns_same_object():
    model = _tiny_model()
    returned = set_mode(model, False)
    assert returned is model


def test_set_mode_toggle_train_eval_train():
    """Toggle: train → eval → train — model.training tracks correctly each step."""
    model = _tiny_model()
    set_mode(model, True)
    assert model.training is True
    set_mode(model, False)
    assert model.training is False
    set_mode(model, True)
    assert model.training is True


def test_set_mode_eval_propagates_to_submodules():
    """eval mode must propagate to all child modules (e.g. Dropout)."""
    model = _tiny_model()
    model.train()
    set_mode(model, False)
    for submodule in model.modules():
        assert not submodule.training, (
            f"Submodule {submodule.__class__.__name__} still in train mode after set_mode(False)"
        )


def test_set_mode_train_propagates_to_submodules():
    """train mode must propagate to all child modules."""
    model = _tiny_model()
    model.eval()
    set_mode(model, True)
    for submodule in model.modules():
        assert submodule.training, (
            f"Submodule {submodule.__class__.__name__} still in eval mode after set_mode(True)"
        )


def test_set_mode_eval_disables_dropout_stochasticity():
    """In eval mode Dropout must be deterministic (same output on two forward passes)."""
    torch.manual_seed(42)
    model = nn.Sequential(nn.Linear(8, 8), nn.Dropout(p=0.5))
    set_mode(model, False)
    x = torch.ones(4, 8)
    with torch.no_grad():
        out1 = model(x)
        out2 = model(x)
    assert torch.allclose(out1, out2), (
        "eval mode: Dropout outputs must be identical across two forward passes"
    )


def test_set_mode_train_enables_dropout_stochasticity():
    """In train mode Dropout introduces randomness (outputs differ without re-seeding)."""
    torch.manual_seed(0)
    model = nn.Sequential(nn.Linear(8, 8), nn.Dropout(p=0.9))
    set_mode(model, True)
    x = torch.ones(16, 8)
    # Two forward passes without re-seeding must differ with high probability
    out1 = model(x)
    out2 = model(x)
    assert not torch.allclose(out1, out2), (
        "train mode: Dropout outputs should differ between consecutive forward passes"
    )


# ---------------------------------------------------------------------------
# run_reproducible_experiment: return type and structure
# ---------------------------------------------------------------------------

def test_run_returns_dict():
    result = run_reproducible_experiment(0)
    assert isinstance(result, dict), "run_reproducible_experiment must return a dict"


def test_run_has_required_keys():
    result = run_reproducible_experiment(1)
    for key in ("final_loss", "n_params", "training_mode"):
        assert key in result, f"result dict missing key '{key}'"


def test_run_final_loss_is_float():
    result = run_reproducible_experiment(2)
    assert isinstance(result["final_loss"], float), "final_loss must be a Python float"


def test_run_n_params_is_int():
    result = run_reproducible_experiment(3)
    assert isinstance(result["n_params"], int), "n_params must be a Python int"


def test_run_training_mode_is_false():
    """After the experiment completes the model must be in eval mode."""
    result = run_reproducible_experiment(4)
    assert result["training_mode"] is False, (
        "training_mode must be False after run_reproducible_experiment"
    )


def test_run_final_loss_positive():
    """MSELoss is always non-negative; with random data it should be > 0."""
    result = run_reproducible_experiment(5)
    assert result["final_loss"] >= 0.0, "final_loss must be non-negative"


# ---------------------------------------------------------------------------
# run_reproducible_experiment: DETERMINISM — same seed → identical results
# ---------------------------------------------------------------------------

def test_determinism_same_seed_same_loss():
    """Core property: same seed → bit-identical final_loss."""
    r1 = run_reproducible_experiment(42)
    r2 = run_reproducible_experiment(42)
    assert r1["final_loss"] == r2["final_loss"], (
        "same seed must produce bit-identical final_loss"
    )


def test_determinism_same_seed_same_n_params():
    r1 = run_reproducible_experiment(7)
    r2 = run_reproducible_experiment(7)
    assert r1["n_params"] == r2["n_params"]


@pytest.mark.parametrize("seed", [0, 1, 13, 42, 100, 999])
def test_determinism_parametrized(seed):
    """Parametrized: many seeds — all must be bit-identical across two runs."""
    r1 = run_reproducible_experiment(seed)
    r2 = run_reproducible_experiment(seed)
    assert r1["final_loss"] == r2["final_loss"], (
        f"seed={seed}: final_loss not deterministic"
    )
    assert r1["training_mode"] is False


def test_determinism_interleaved_seeds():
    """Interleaved calls do not interfere: reseed resets state fully each time."""
    r_a1 = run_reproducible_experiment(11)
    r_b1 = run_reproducible_experiment(22)
    r_a2 = run_reproducible_experiment(11)
    r_b2 = run_reproducible_experiment(22)
    assert r_a1["final_loss"] == r_a2["final_loss"], "seed=11 not deterministic after interleave"
    assert r_b1["final_loss"] == r_b2["final_loss"], "seed=22 not deterministic after interleave"


# ---------------------------------------------------------------------------
# run_reproducible_experiment: DISTINGUISHABILITY — different seeds → different results
# ---------------------------------------------------------------------------

def test_distinguishability_different_seeds():
    """Different seeds must (almost certainly) give different final_loss values."""
    r0 = run_reproducible_experiment(0)
    r1 = run_reproducible_experiment(1)
    assert r0["final_loss"] != r1["final_loss"], (
        "different seeds should produce different final_loss (identical is astronomically unlikely)"
    )


@pytest.mark.parametrize("s1,s2", [(0, 1), (42, 43), (100, 200), (7, 77)])
def test_distinguishability_parametrized(s1, s2):
    r1 = run_reproducible_experiment(s1)
    r2 = run_reproducible_experiment(s2)
    assert r1["final_loss"] != r2["final_loss"], (
        f"seeds {s1} and {s2} produced identical final_loss — seeding likely broken"
    )


# ---------------------------------------------------------------------------
# run_reproducible_experiment: n_params oracle check
# ---------------------------------------------------------------------------

def test_n_params_matches_default_architecture():
    """Default architecture: Linear(8,16) + Linear(16,1) — verify against oracle.

    Oracle: (8*16 + 16) + (16*1 + 1) = 144 + 17 = 161
    """
    result = run_reproducible_experiment(0)
    expected = (8 * 16 + 16) + (16 * 1 + 1)   # 144 + 17 = 161
    assert result["n_params"] == expected, (
        f"n_params={result['n_params']}, expected {expected} for default architecture"
    )


def test_n_params_custom_architecture():
    """Custom n_features and hidden: oracle must match."""
    n_features, hidden = 4, 8
    result = run_reproducible_experiment(seed=0, n_features=n_features, hidden=hidden)
    # Linear(4,8): 4*8+8=40; Linear(8,1): 8*1+1=9 → 49
    expected = (n_features * hidden + hidden) + (hidden * 1 + 1)
    assert result["n_params"] == expected, (
        f"n_params={result['n_params']}, expected {expected}"
    )


def test_n_params_agrees_with_count_parameters():
    """n_params in the result must agree with count_parameters on the same architecture."""
    set_seed(55)
    manual_model = nn.Sequential(
        nn.Linear(8, 16),
        nn.ReLU(),
        nn.Linear(16, 1),
    )
    oracle = count_parameters(manual_model)
    result = run_reproducible_experiment(0)
    assert result["n_params"] == oracle, (
        f"run returned n_params={result['n_params']}, count_parameters oracle={oracle}"
    )


# ---------------------------------------------------------------------------
# run_reproducible_experiment: custom parameters
# ---------------------------------------------------------------------------

def test_custom_n_steps_decreases_loss():
    """More training steps should (almost certainly) reduce the loss."""
    r_few = run_reproducible_experiment(seed=42, n_steps=1)
    r_many = run_reproducible_experiment(seed=42, n_steps=50)
    # With the same seed the starting weights are identical; more steps → lower loss
    assert r_many["final_loss"] < r_few["final_loss"], (
        "More training steps should reduce final_loss for the same seed"
    )


def test_custom_lr_affects_loss():
    """Different learning rates should produce different final_loss values."""
    r_slow = run_reproducible_experiment(seed=10, lr=0.0001, n_steps=10)
    r_fast = run_reproducible_experiment(seed=10, lr=0.1, n_steps=10)
    assert r_slow["final_loss"] != r_fast["final_loss"], (
        "Different learning rates must yield different final_loss"
    )


def test_zero_steps_gives_untrained_loss():
    """With n_steps=0 the model is untrained; two runs with same seed still match."""
    r1 = run_reproducible_experiment(seed=77, n_steps=0)
    r2 = run_reproducible_experiment(seed=77, n_steps=0)
    assert r1["final_loss"] == r2["final_loss"]
    assert r1["training_mode"] is False


# ---------------------------------------------------------------------------
# Integration: set_mode + run_reproducible_experiment together
# ---------------------------------------------------------------------------

def test_set_mode_does_not_break_determinism():
    """Calling set_mode between experiment runs must not affect determinism."""
    tmp_model = nn.Linear(4, 4)
    set_mode(tmp_model, True)   # side-effect on a throwaway model
    r1 = run_reproducible_experiment(99)
    set_mode(tmp_model, False)
    r2 = run_reproducible_experiment(99)
    assert r1["final_loss"] == r2["final_loss"], (
        "set_mode calls between runs must not affect run_reproducible_experiment determinism"
    )


def test_experiment_uses_seeded_init_not_external_state():
    """External torch state before the call must NOT affect experiment output.

    Strategy: advance the global torch RNG by drawing random numbers, then call
    run_reproducible_experiment — it must still match a fresh run from a clean state.
    """
    torch.manual_seed(0)
    torch.randn(1000)   # advance global RNG significantly

    r_after_noise = run_reproducible_experiment(42)
    r_clean = run_reproducible_experiment(42)   # fresh call, same seed
    assert r_after_noise["final_loss"] == r_clean["final_loss"], (
        "run_reproducible_experiment must re-seed internally and be immune to external RNG state"
    )
