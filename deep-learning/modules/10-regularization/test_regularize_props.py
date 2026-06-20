# Randomized / property-based tests for module 10 — Regularization.
#
# These tests are COMPLEMENTARY to test_regularize.py (no duplication of
# its fixed-input cases).  They use:
#   - PyTorch as an oracle where applicable
#   - Numerical finite-difference gradient checks
#   - Invariant / property checks on random inputs
#   - Edge-case probing (zeros, negatives, single rows, large magnitudes)
#
# All calls to the module happen INSIDE test functions so this file
# imports / collects cleanly against a raise-NotImplementedError stub.

import numpy as np
import pytest
import torch
import torch.nn.functional as F

from regularize import (
    dropout_mask,
    l2_penalty,
    should_early_stop,
    train_val_split,
)


# ========================================================================== #
#  Helper utilities                                                            #
# ========================================================================== #

def _random_X_y(n, d, seed):
    rng = np.random.default_rng(seed)
    X = rng.normal(size=(n, d))
    y = np.arange(n)
    return X, y


# ========================================================================== #
#  train_val_split — randomised properties                                    #
# ========================================================================== #

class TestTrainValSplitProps:

    def test_sizes_vary_with_val_frac(self):
        """n_val = round(N * val_frac) for several fracs and N values."""
        np.random.seed(10)
        for n, d, val_frac, seed in [
            (30, 4, 0.2, 1),
            (50, 5, 0.33, 2),
            (100, 2, 0.1, 3),
            (7,  3, 0.5, 4),
        ]:
            X, y = _random_X_y(n, d, seed)
            X_tr, y_tr, X_val, y_val = train_val_split(X, y, val_frac, seed)
            expected_n_val = int(round(n * val_frac))
            assert X_val.shape[0] == expected_n_val, (
                f"n={n}, val_frac={val_frac}: expected {expected_n_val} val rows, "
                f"got {X_val.shape[0]}"
            )
            assert X_tr.shape[0] == n - expected_n_val
            assert X_tr.shape[1] == d
            assert X_val.shape[1] == d
            assert y_tr.shape[0] == n - expected_n_val
            assert y_val.shape[0] == expected_n_val

    def test_no_data_loss_across_shapes(self):
        """Union of y_tr and y_val == full index set, for many random configs."""
        np.random.seed(20)
        for n, val_frac, seed in [
            (25, 0.2, 5), (40, 0.4, 6), (11, 0.5, 7), (100, 0.15, 8),
        ]:
            X, y = _random_X_y(n, 3, seed)
            _, y_tr, _, y_val = train_val_split(X, y, val_frac, seed)
            assert sorted(np.concatenate([y_tr, y_val]).tolist()) == list(range(n))

    def test_no_duplicate_rows_in_split(self):
        """No row should appear in both train and val."""
        np.random.seed(30)
        for n, val_frac, seed in [(20, 0.3, 9), (50, 0.2, 10)]:
            X, y = _random_X_y(n, 4, seed)
            _, y_tr, _, y_val = train_val_split(X, y, val_frac, seed)
            tr_set = set(y_tr.tolist())
            val_set = set(y_val.tolist())
            assert tr_set.isdisjoint(val_set), "Rows appear in both train and val!"

    def test_X_y_rows_stay_paired(self):
        """After split X[i] row must still correspond to y[i] (identity check via label==row-index)."""
        np.random.seed(40)
        for n, val_frac, seed in [(30, 0.3, 11), (40, 0.25, 12), (15, 0.5, 13)]:
            X, y = _random_X_y(n, 3, seed)
            X_tr, y_tr, X_val, y_val = train_val_split(X, y, val_frac, seed)
            for X_part, y_part in [(X_tr, y_tr), (X_val, y_val)]:
                for row_vec, label in zip(X_part, y_part):
                    assert np.allclose(row_vec, X[label])

    def test_different_seeds_give_different_splits(self):
        """Two different seeds must (almost surely) produce different val-index sets."""
        np.random.seed(50)
        X, y = _random_X_y(50, 5, 0)
        _, _, _, y_val_a = train_val_split(X, y, 0.2, seed=100)
        _, _, _, y_val_b = train_val_split(X, y, 0.2, seed=999)
        assert not np.array_equal(y_val_a, y_val_b), (
            "Two different seeds produced identical val splits — very unlikely unless shuffle is broken."
        )

    def test_determinism_many_seeds(self):
        """Bit-exact reproducibility for 8 different seeds."""
        np.random.seed(60)
        X, y = _random_X_y(40, 5, 0)
        for seed in range(8):
            a = train_val_split(X, y, 0.25, seed=seed)
            b = train_val_split(X, y, 0.25, seed=seed)
            for arr_a, arr_b in zip(a, b):
                assert np.array_equal(arr_a, arr_b), f"Not deterministic for seed={seed}"

    def test_single_example_val(self):
        """Edge case: val_frac such that n_val = 1."""
        n = 10
        X, y = _random_X_y(n, 2, 0)
        # val_frac = 0.1 => round(10 * 0.1) = 1
        X_tr, y_tr, X_val, y_val = train_val_split(X, y, val_frac=0.1, seed=0)
        assert X_val.shape == (1, 2)
        assert X_tr.shape == (9, 2)

    def test_val_frac_zero_gives_empty_val(self):
        """val_frac=0.0 => all data in train, empty val (n_val=0)."""
        X, y = _random_X_y(20, 3, 0)
        X_tr, y_tr, X_val, y_val = train_val_split(X, y, val_frac=0.0, seed=0)
        assert X_val.shape[0] == 0
        assert X_tr.shape[0] == 20

    def test_2d_y_pairs_stay_together(self):
        """With (N, K) targets each row of y_tr must match the original X row label."""
        np.random.seed(70)
        n, d, k = 20, 4, 3
        rng = np.random.default_rng(70)
        X = rng.normal(size=(n, d))
        y2d = np.tile(np.arange(n)[:, None], (1, k))   # y2d[i] = [i, i, i]
        X_tr, y_tr, X_val, y_val = train_val_split(X, y2d, val_frac=0.3, seed=3)
        for X_part, y_part in [(X_tr, y_tr), (X_val, y_val)]:
            for row_vec, label_row in zip(X_part, y_part):
                label = label_row[0]  # all columns equal
                assert np.allclose(row_vec, X[label])


# ========================================================================== #
#  l2_penalty — oracle vs PyTorch + invariants                               #
# ========================================================================== #

class TestL2PenaltyProps:

    def test_oracle_1d_vs_torch(self):
        """numpy l2_penalty must match torch manual computation on random 1-D weights."""
        torch.manual_seed(1)
        np.random.seed(1)
        for lam in [0.01, 0.1, 1.0, 5.0]:
            w_np = np.random.randn(32)
            w_t = torch.tensor(w_np, dtype=torch.float64)
            expected = float(lam * (w_t ** 2).sum().item())
            got = l2_penalty(w_np, lam)
            assert abs(got - expected) < 1e-10, (
                f"lam={lam}: oracle={expected:.8f}, got={got:.8f}"
            )

    def test_oracle_2d_vs_torch(self):
        """Same check for 2-D weight matrices (e.g. FC layer weights)."""
        torch.manual_seed(2)
        np.random.seed(2)
        for shape, lam in [((16, 8), 0.05), ((3, 100), 2.0), ((10, 10), 0.5)]:
            w_np = np.random.randn(*shape)
            w_t = torch.tensor(w_np, dtype=torch.float64)
            expected = float(lam * (w_t ** 2).sum().item())
            got = l2_penalty(w_np, lam)
            assert abs(got - expected) < 1e-10

    def test_oracle_3d_and_scalar(self):
        """3-D weights (conv filters) and scalar weights still produce correct penalty."""
        np.random.seed(3)
        for shape, lam in [((4, 3, 3), 0.1), ((), 1.0)]:
            w_np = np.random.randn(*shape)
            expected = float(lam * np.sum(w_np ** 2))
            got = l2_penalty(w_np, lam)
            assert abs(got - expected) < 1e-12

    def test_nonnegative_for_any_weights(self):
        """l2_penalty >= 0 always (it is a sum of squares)."""
        np.random.seed(4)
        for _ in range(10):
            w = np.random.randn(np.random.randint(1, 50))
            lam = np.random.uniform(0.0, 10.0)
            assert l2_penalty(w, lam) >= 0.0

    def test_scales_quadratically_with_weight_magnitude(self):
        """Doubling weights => penalty grows by factor 4 (lam * (2w)^2 = 4 * lam * w^2)."""
        np.random.seed(5)
        w = np.random.randn(20)
        lam = 0.3
        p1 = l2_penalty(w, lam)
        p2 = l2_penalty(2.0 * w, lam)
        assert abs(p2 - 4.0 * p1) < 1e-10

    def test_scales_linearly_with_lambda(self):
        """Doubling lam => penalty doubles."""
        np.random.seed(6)
        w = np.random.randn(15)
        p1 = l2_penalty(w, lam=0.5)
        p2 = l2_penalty(w, lam=1.0)
        assert abs(p2 - 2.0 * p1) < 1e-10

    def test_all_zeros_penalty_is_zero(self):
        """Zero weights always give zero penalty regardless of lam."""
        for lam in [0.0, 0.1, 100.0]:
            assert l2_penalty(np.zeros(10), lam) == 0.0

    def test_returns_python_float_always(self):
        """Return type must be Python float (not np.float64) for all shapes."""
        np.random.seed(7)
        for shape in [(5,), (4, 4), (2, 3, 4)]:
            out = l2_penalty(np.random.randn(*shape), lam=0.1)
            assert isinstance(out, float), f"Expected float, got {type(out)}"

    def test_large_magnitude_no_overflow(self):
        """Extremely large weights should not cause inf (use a small lam to stay finite)."""
        w = np.array([1e100, -1e100])
        # sum of squares = 2e200, lam*sum = 2e198 — finite
        result = l2_penalty(w, lam=1e-2)
        assert np.isfinite(result)

    def test_negative_weights_same_as_positive(self):
        """Penalty is symmetric: l2_penalty(w) == l2_penalty(-w)."""
        np.random.seed(8)
        w = np.random.randn(20)
        assert l2_penalty(w, 0.7) == pytest.approx(l2_penalty(-w, 0.7), rel=1e-12)

    def test_gradient_via_finite_difference(self):
        """dl/dw_i = 2*lam*w_i — verify with finite differences (central diff, h=1e-5)."""
        np.random.seed(9)
        w = np.random.randn(8)
        lam = 0.3
        h = 1e-5
        analytic_grad = 2.0 * lam * w
        fd_grad = np.zeros_like(w)
        for i in range(len(w)):
            wp = w.copy(); wp[i] += h
            wm = w.copy(); wm[i] -= h
            fd_grad[i] = (l2_penalty(wp, lam) - l2_penalty(wm, lam)) / (2 * h)
        assert np.allclose(analytic_grad, fd_grad, atol=1e-6), (
            f"Finite-diff grad mismatch:\nanalytic={analytic_grad}\nfd={fd_grad}"
        )


# ========================================================================== #
#  dropout_mask — oracle vs expected distribution + invariants               #
# ========================================================================== #

class TestDropoutMaskProps:

    def test_only_two_values_present(self):
        """Mask must only contain 0.0 or 1/(1-p) for several p values."""
        np.random.seed(11)
        for p in [0.1, 0.3, 0.5, 0.7, 0.8]:
            scale = 1.0 / (1.0 - p)
            m = dropout_mask((50, 50), p=p, seed=11)
            is_zero = np.isclose(m, 0.0)
            is_scale = np.isclose(m, scale)
            assert np.all(is_zero | is_scale), (
                f"p={p}: found values other than 0.0 and {scale:.4f}"
            )

    def test_mean_close_to_one_large_mask(self):
        """Inverted-dropout invariant: E[mask] = 1.0 for large arrays."""
        np.random.seed(12)
        for p in [0.1, 0.3, 0.5, 0.7]:
            m = dropout_mask((500, 500), p=p, seed=12)
            assert abs(m.mean() - 1.0) < 0.03, (
                f"p={p}: mean={m.mean():.4f}, expected ~1.0"
            )

    def test_drop_fraction_close_to_p_large_mask(self):
        """Fraction of zeros in large mask should be within 3% of p."""
        np.random.seed(13)
        for p in [0.2, 0.4, 0.6]:
            m = dropout_mask((500, 500), p=p, seed=13)
            frac_zero = np.isclose(m, 0.0).mean()
            assert abs(frac_zero - p) < 0.03, (
                f"p={p}: frac_zero={frac_zero:.4f}, expected ~{p}"
            )

    def test_dtype_is_float(self):
        """Output must be floating-point dtype."""
        m = dropout_mask((10, 10), p=0.5, seed=0)
        assert np.issubdtype(m.dtype, np.floating), f"Expected float dtype, got {m.dtype}"

    def test_shape_preserved_various(self):
        """Output shape == requested shape for several 1-D and 3-D cases."""
        for shape in [(20,), (3, 4, 5), (1, 100), (64, 64)]:
            m = dropout_mask(shape, p=0.3, seed=0)
            assert m.shape == shape

    def test_p_zero_all_ones_various_shapes(self):
        """p=0 must give all-ones mask for many shapes and seeds."""
        for shape, seed in [((10,), 0), ((5, 5), 1), ((3, 4, 5), 2)]:
            m = dropout_mask(shape, p=0.0, seed=seed)
            assert np.allclose(m, 1.0), f"p=0, shape={shape}: not all ones"

    def test_scale_value_exact(self):
        """Active elements must equal exactly 1/(1-p) (not approximately)."""
        for p in [0.2, 0.5, 0.75]:
            m = dropout_mask((100, 100), p=p, seed=42)
            active = m[~np.isclose(m, 0.0)]
            expected_scale = 1.0 / (1.0 - p)
            assert np.allclose(active, expected_scale, rtol=1e-10), (
                f"p={p}: active elements should all be {expected_scale:.6f}"
            )

    def test_different_seeds_give_different_masks(self):
        """Two different seeds must produce different masks (almost surely)."""
        m1 = dropout_mask((100, 100), p=0.5, seed=0)
        m2 = dropout_mask((100, 100), p=0.5, seed=1)
        assert not np.array_equal(m1, m2)

    def test_single_element_shape(self):
        """Shape (1,) must work without errors."""
        for p in [0.0, 0.5]:
            m = dropout_mask((1,), p=p, seed=7)
            assert m.shape == (1,)

    def test_p_extreme_high_drop_rate(self):
        """p=0.9: about 90% should be zeros in a large mask."""
        m = dropout_mask((1000, 100), p=0.9, seed=99)
        frac_zero = np.isclose(m, 0.0).mean()
        assert abs(frac_zero - 0.9) < 0.03

    def test_apply_mask_preserves_mean_activations(self):
        """Applying the mask to a constant activation vector preserves the expected mean."""
        np.random.seed(14)
        activations = np.ones((500, 500))
        p = 0.4
        m = dropout_mask(activations.shape, p=p, seed=14)
        result = activations * m
        # mean of result should be ~1.0 (inverted dropout guarantee)
        assert abs(result.mean() - 1.0) < 0.04

    def test_determinism_multiple_seeds(self):
        """For each of 8 different seeds the result is bit-exact reproducible."""
        for seed in range(8):
            a = dropout_mask((30, 30), p=0.4, seed=seed)
            b = dropout_mask((30, 30), p=0.4, seed=seed)
            assert np.array_equal(a, b), f"Not deterministic for seed={seed}"


# ========================================================================== #
#  should_early_stop — randomised sequences + boundary cases                 #
# ========================================================================== #

class TestShouldEarlyStopProps:

    def test_always_false_before_patience_plus_one(self):
        """With fewer than patience+1 epochs, always False regardless of values."""
        np.random.seed(15)
        for patience in [1, 2, 5]:
            # sequences of length < patience+1
            for length in range(1, patience + 1):
                losses = list(np.random.rand(length))
                result = should_early_stop(losses, patience)
                assert result is False, (
                    f"patience={patience}, len={length}: expected False, got {result}"
                )

    def test_monotone_decreasing_never_stops(self):
        """A strictly decreasing loss sequence should NEVER trigger early stop."""
        np.random.seed(16)
        for patience in [1, 2, 3, 5]:
            losses = sorted(np.random.rand(patience + 5).tolist(), reverse=True)
            assert should_early_stop(losses, patience) is False, (
                f"patience={patience}: monotone decrease should not stop"
            )

    def test_plateau_after_min_triggers(self):
        """One sharp drop followed by flat plateau should trigger after patience epochs."""
        np.random.seed(17)
        for patience in [2, 3, 4]:
            losses = [1.0, 0.3] + [0.5] * patience  # min at index 1; patience flat epochs after
            assert should_early_stop(losses, patience) is True, (
                f"patience={patience}: plateau should trigger stop"
            )

    def test_one_too_few_epochs_on_plateau(self):
        """Plateau of length patience-1 should NOT trigger (one short)."""
        for patience in [2, 3, 5]:
            losses = [1.0, 0.3] + [0.5] * (patience - 1)
            assert should_early_stop(losses, patience) is False, (
                f"patience={patience}: plateau of {patience-1} should not stop"
            )

    def test_improvement_at_last_epoch_resets(self):
        """If a new minimum occurs at the very last epoch, must not stop."""
        np.random.seed(18)
        for patience in [2, 3]:
            # rising then sudden new minimum
            losses = [1.0, 0.9, 0.8, 0.7, 0.5]   # last is new minimum
            assert should_early_stop(losses, patience) is False

    def test_returns_bool_not_numpy(self):
        """Return type must be Python bool (not np.bool_)."""
        result = should_early_stop([1.0, 0.9, 0.8, 0.85], patience=2)
        assert type(result) is bool, f"Expected bool, got {type(result)}"

    def test_single_loss_always_false(self):
        """List of length 1 is always False for any patience."""
        for patience in [1, 2, 10]:
            assert should_early_stop([0.5], patience) is False

    def test_exactly_at_patience_boundary(self):
        """Exactly patience epochs without improvement => True; one fewer => False."""
        for patience in [1, 2, 3, 4]:
            # best at index 0, then `patience` worse values → total length = patience+1
            losses_true = [0.1] + [0.9] * patience
            assert should_early_stop(losses_true, patience) is True, (
                f"Exactly {patience} no-improvement epochs should stop"
            )
            # best at index 0, then `patience-1` worse values → too short
            losses_false = [0.1] + [0.9] * (patience - 1)
            assert should_early_stop(losses_false, patience) is False, (
                f"Only {patience-1} no-improvement epochs should NOT stop"
            )

    def test_best_in_middle_correct_count(self):
        """Best in the middle: count is measured from the best, not from the start."""
        # best at index 2; patience=3 means we need 3 epochs after → indices 3,4,5
        losses = [1.0, 0.8, 0.5, 0.7, 0.9, 1.1]   # 3 epochs after best
        assert should_early_stop(losses, patience=3) is True

        # same but only 2 epochs after best
        losses2 = [1.0, 0.8, 0.5, 0.7, 0.9]
        assert should_early_stop(losses2, patience=3) is False

    def test_noisy_loss_with_eventual_improvement(self):
        """A noisy curve that eventually reaches a new minimum should not stop."""
        losses = [1.0, 0.9, 1.1, 0.95, 0.85, 1.2, 0.80]  # new min at last
        for patience in [2, 3]:
            assert should_early_stop(losses, patience) is False

    def test_all_equal_values(self):
        """All values identical: best is at index 0, so epochs without improvement = n-1."""
        for patience in [1, 2, 3]:
            losses = [0.5] * (patience + 2)  # n-1 = patience+1 >= patience → stop
            assert should_early_stop(losses, patience) is True

    def test_patience_one_stops_after_one_no_improve(self):
        """patience=1 means any single non-improving epoch triggers stop."""
        assert should_early_stop([0.5, 0.6], patience=1) is True
        assert should_early_stop([0.5, 0.4], patience=1) is False

    def test_random_sequences_consistency(self):
        """Randomly generated sequences: manual reference vs function output."""
        np.random.seed(19)
        for _ in range(50):
            length = np.random.randint(2, 20)
            patience = np.random.randint(1, 6)
            losses = np.random.rand(length).tolist()
            # reference implementation
            if length < patience + 1:
                expected = False
            else:
                best = int(np.argmin(losses))
                epochs_no_improve = length - 1 - best
                expected = epochs_no_improve >= patience
            got = should_early_stop(losses, patience)
            assert got == expected, (
                f"losses={losses}, patience={patience}: expected {expected}, got {got}"
            )


# ========================================================================== #
#  Cross-function integration: dropout applied to activations                 #
# ========================================================================== #

class TestIntegration:

    def test_dropout_zero_preserves_forward_pass(self):
        """p=0 dropout mask applied to any activations leaves them unchanged."""
        np.random.seed(21)
        activations = np.random.randn(10, 20)
        m = dropout_mask(activations.shape, p=0.0, seed=21)
        assert np.allclose(activations * m, activations)

    def test_l2_penalty_positive_for_random_weights(self):
        """For random non-zero weights and positive lam, l2_penalty is strictly positive."""
        np.random.seed(22)
        for _ in range(10):
            w = np.random.randn(np.random.randint(1, 30))
            # avoid the improbable all-zeros case
            if np.allclose(w, 0.0):
                continue
            lam = np.random.uniform(0.01, 5.0)
            assert l2_penalty(w, lam) > 0.0

    def test_split_then_l2_on_random_data(self):
        """Split data, compute l2_penalty on a dummy weight — shapes and types are consistent."""
        np.random.seed(23)
        X = np.random.randn(40, 6)
        y = np.arange(40)
        X_tr, _, X_val, _ = train_val_split(X, y, val_frac=0.2, seed=23)
        w = np.random.randn(6)
        penalty = l2_penalty(w, lam=0.1)
        assert isinstance(penalty, float)
        assert penalty >= 0.0
        assert X_tr.shape[1] == 6
        assert X_val.shape[1] == 6
