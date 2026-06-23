# Randomised / property-based tests for module 13 — CNN.
# These complement the fixed cases in test_cnn.py.
# All calls to the module live inside test functions so the file
# imports/collects cleanly against unfilled stubs (raise NotImplementedError).

import math

import pytest
import torch
import torch.nn as nn

from cnn import (
    conv_output_size,
    SmallCNN,
    train_step,
    receptive_field,
    augment_batch,
)


# ---------------------------------------------------------------------------
# Helpers
# ---------------------------------------------------------------------------

def _toy_images(seed: int, n_per_class: int = 8):
    """Two-class 8×8 dataset mirroring the one in test_cnn.py."""
    g = torch.Generator().manual_seed(seed)
    imgs, labels = [], []
    for _ in range(n_per_class):
        a = 0.3 * torch.randn(1, 8, 8, generator=g)
        a[:, :4, :] += 2.0
        imgs.append(a)
        labels.append(0)
    for _ in range(n_per_class):
        b = 0.3 * torch.randn(1, 8, 8, generator=g)
        b[:, 4:, :] += 2.0
        imgs.append(b)
        labels.append(1)
    return torch.stack(imgs, dim=0), torch.tensor(labels, dtype=torch.long)


# ---------------------------------------------------------------------------
# conv_output_size — randomised formula agreement with nn.Conv2d
# ---------------------------------------------------------------------------

class TestConvOutputSizeRandomised:
    """Our formula must agree with what torch actually produces."""

    @pytest.mark.parametrize("seed", [0, 1, 2, 3, 7])
    def test_matches_torch_conv2d_random_params(self, seed: int):
        """Loop over random (in_size, kernel, stride, pad) combos."""
        rng = torch.Generator()
        rng.manual_seed(seed * 13 + 17)

        for _ in range(20):
            in_h = int(torch.randint(4, 32, (1,), generator=rng).item())
            k = int(torch.randint(1, min(in_h, 8), (1,), generator=rng).item())
            s = int(torch.randint(1, 4, (1,), generator=rng).item())
            # pad must keep (in_h + 2*pad - k) >= 0
            max_pad = (in_h - 1) // 2  # conservative upper bound
            p = int(torch.randint(0, max(max_pad, 1), (1,), generator=rng).item())

            conv = nn.Conv2d(1, 1, kernel_size=k, stride=s, padding=p)
            x = torch.randn(1, 1, in_h, in_h)
            expected = conv(x).shape[-1]
            got = conv_output_size(in_h, k, s, p)
            assert got == expected, (
                f"seed={seed} in={in_h} k={k} s={s} p={p}: "
                f"formula={got} torch={expected}"
            )

    def test_formula_is_floor_division(self):
        """The formula must use floor (//), not ceil or round."""
        # (7 + 0 - 3) / 2 = 2.0 exactly, floor=2
        assert conv_output_size(7, 3, 2, 0) == 3   # (7-3)//2 + 1 = 3
        # (6 + 0 - 3) / 2 = 1.5 -> floor = 1 -> result = 2
        assert conv_output_size(6, 3, 2, 0) == 2   # (6-3)//2 + 1 = 2
        # (5 + 0 - 3) / 2 = 1.0 -> floor = 1 -> result = 2
        assert conv_output_size(5, 3, 2, 0) == 2   # (5-3)//2 + 1 = 2

    def test_return_type_is_int_always(self):
        """Must return Python int, not float or tensor."""
        for (i, k, s, p) in [(8, 3, 1, 0), (16, 5, 2, 2), (4, 1, 1, 0)]:
            result = conv_output_size(i, k, s, p)
            assert type(result) is int, f"Expected int, got {type(result)}"

    def test_same_padding_preserves_size(self):
        """For odd kernel k, pad=(k-1)//2, stride=1 must keep size unchanged."""
        for in_size in [4, 8, 12, 16]:
            for k in [1, 3, 5, 7]:
                p = (k - 1) // 2
                out = conv_output_size(in_size, k, stride=1, pad=p)
                assert out == in_size, (
                    f"same-padding failed: in={in_size} k={k} p={p} -> out={out}"
                )

    def test_large_input_no_overflow(self):
        """Large inputs (e.g. 1024) must not overflow or return wrong type."""
        result = conv_output_size(1024, 7, 2, 3)
        assert isinstance(result, int)
        assert result == (1024 + 6 - 7) // 2 + 1

    def test_monotone_in_input_size(self):
        """Larger input with same kernel/stride/pad gives larger or equal output."""
        k, s, p = 3, 1, 0
        sizes = list(range(k, 30))
        outputs = [conv_output_size(sz, k, s, p) for sz in sizes]
        assert outputs == sorted(outputs), "output must be non-decreasing in input size"

    def test_monotone_decreasing_in_kernel(self):
        """Bigger kernel on fixed input gives smaller or equal output (no pad, stride=1)."""
        in_size = 20
        outputs = [conv_output_size(in_size, k, 1, 0) for k in range(1, in_size)]
        assert outputs == sorted(outputs, reverse=True), (
            "output must be non-increasing as kernel grows"
        )


# ---------------------------------------------------------------------------
# SmallCNN — shape, dtype, output properties
# ---------------------------------------------------------------------------

class TestSmallCNNShapes:
    """Output shape must be exactly (N, num_classes) regardless of N or classes."""

    @pytest.mark.parametrize("n", [1, 2, 5, 16, 32])
    @pytest.mark.parametrize("seed", [0, 42, 99])
    def test_output_shape_various_batch_sizes(self, n: int, seed: int):
        torch.manual_seed(seed)
        model = SmallCNN()
        x = torch.randn(n, 1, 8, 8)
        out = model(x)
        assert tuple(out.shape) == (n, 2)

    @pytest.mark.parametrize("num_classes", [2, 3, 5, 10])
    def test_output_shape_various_num_classes(self, num_classes: int):
        torch.manual_seed(7)
        model = SmallCNN(num_classes=num_classes)
        x = torch.randn(4, 1, 8, 8)
        out = model(x)
        assert tuple(out.shape) == (4, num_classes)

    def test_output_dtype_is_float32(self):
        """Logits must be float32."""
        torch.manual_seed(3)
        model = SmallCNN()
        x = torch.randn(4, 1, 8, 8)
        out = model(x)
        assert out.dtype == torch.float32

    def test_output_is_not_probabilities(self):
        """Rows must NOT sum to 1 (no softmax on output)."""
        torch.manual_seed(5)
        model = SmallCNN()
        x = torch.randn(10, 1, 8, 8)
        out = model(x)
        row_sums = out.sum(dim=1)
        # If softmax were applied, all row sums would equal 1.0
        assert not torch.allclose(row_sums, torch.ones_like(row_sums), atol=1e-3), (
            "Rows sum to 1 — model appears to apply softmax (it must not)"
        )

    def test_logits_can_be_negative(self):
        """Raw logits must include negative values — softmax output never can."""
        torch.manual_seed(11)
        model = SmallCNN()
        x = torch.randn(16, 1, 8, 8)
        out = model(x)
        assert (out < 0).any(), "No negative values found — looks like softmax was applied"

    def test_batch_independence(self):
        """Each row of the output must depend only on the corresponding input row."""
        torch.manual_seed(20)
        model = SmallCNN()
        model.eval()

        x_single_0 = torch.randn(1, 1, 8, 8)
        x_single_1 = torch.randn(1, 1, 8, 8)
        x_batch = torch.cat([x_single_0, x_single_1], dim=0)   # (2, 1, 8, 8)

        out_batch = model(x_batch)
        out_0 = model(x_single_0)
        out_1 = model(x_single_1)

        assert torch.allclose(out_batch[0:1], out_0, atol=1e-5), (
            "Batch row 0 differs from single-image forward — batch independence broken"
        )
        assert torch.allclose(out_batch[1:2], out_1, atol=1e-5), (
            "Batch row 1 differs from single-image forward — batch independence broken"
        )

    def test_deterministic_given_same_seed(self):
        """Same seed → same weights → identical outputs."""
        x = torch.randn(4, 1, 8, 8)
        torch.manual_seed(123)
        out_a = SmallCNN()(x)
        torch.manual_seed(123)
        out_b = SmallCNN()(x)
        assert torch.allclose(out_a, out_b)

    def test_different_seeds_give_different_outputs(self):
        """Different seeds → different weights → at least one logit differs."""
        x = torch.randn(4, 1, 8, 8)
        torch.manual_seed(1)
        out_a = SmallCNN()(x)
        torch.manual_seed(999)
        out_b = SmallCNN()(x)
        assert not torch.allclose(out_a, out_b)

    def test_conv_layer_uses_relu_nonneg_feature_maps(self):
        """After conv+relu, all activations must be >= 0."""
        torch.manual_seed(30)
        model = SmallCNN()
        model.eval()
        x = torch.randn(4, 1, 8, 8)
        # Access internal activations by running through conv + relu
        with torch.no_grad():
            after_relu = model.relu(model.conv(x))
        assert (after_relu >= 0).all(), "ReLU output must be non-negative"

    def test_zero_input_produces_finite_output(self):
        """All-zero input must not produce NaN or Inf logits."""
        torch.manual_seed(40)
        model = SmallCNN()
        x = torch.zeros(2, 1, 8, 8)
        out = model(x)
        assert torch.isfinite(out).all(), "Zero input produced non-finite logits"

    def test_large_magnitude_input_produces_finite_output(self):
        """Large-magnitude input must not produce NaN or Inf logits."""
        torch.manual_seed(50)
        model = SmallCNN()
        x = torch.ones(2, 1, 8, 8) * 1e4
        out = model(x)
        assert torch.isfinite(out).all(), "Large input produced non-finite logits"

    def test_negative_input_produces_finite_output(self):
        """All-negative input must not cause errors or NaN."""
        torch.manual_seed(55)
        model = SmallCNN()
        x = -1e3 * torch.ones(2, 1, 8, 8)
        out = model(x)
        assert torch.isfinite(out).all()

    @pytest.mark.parametrize("seed", range(5))
    def test_output_shape_single_image_various_seeds(self, seed: int):
        """N=1 is a common edge case — must work with all seeds."""
        torch.manual_seed(seed * 7)
        model = SmallCNN()
        x = torch.randn(1, 1, 8, 8)
        out = model(x)
        assert tuple(out.shape) == (1, 2)


# ---------------------------------------------------------------------------
# SmallCNN — parameter count and gradient flow
# ---------------------------------------------------------------------------

class TestSmallCNNParameters:
    """Parameters must exist, be float32, and receive gradients on backward."""

    def test_parameter_count(self):
        """Default SmallCNN should have exactly the expected number of parameters.

        Conv2d(1, 4, 3): 4*1*3*3 weights + 4 biases = 40
        Linear(36, 2):   36*2 weights + 2 biases     = 74
        Total: 114
        """
        model = SmallCNN()
        total = sum(p.numel() for p in model.parameters())
        assert total == 114, f"Expected 114 parameters, got {total}"

    @pytest.mark.parametrize("num_classes", [2, 5])
    def test_parameter_count_custom_classes(self, num_classes: int):
        """Linear layer size scales with num_classes."""
        model = SmallCNN(num_classes=num_classes)
        # Conv: 40; Linear: 36*num_classes + num_classes
        expected = 40 + 36 * num_classes + num_classes
        total = sum(p.numel() for p in model.parameters())
        assert total == expected, f"Expected {expected} params, got {total}"

    def test_all_params_float32(self):
        model = SmallCNN()
        for p in model.parameters():
            assert p.dtype == torch.float32, f"Parameter dtype is {p.dtype}"

    def test_all_params_require_grad(self):
        model = SmallCNN()
        for p in model.parameters():
            assert p.requires_grad, "A parameter has requires_grad=False"

    def test_gradients_nonzero_after_backward(self):
        """All parameters must receive non-zero gradients after a typical backward."""
        torch.manual_seed(60)
        model = SmallCNN()
        loss_fn = nn.CrossEntropyLoss()
        x = torch.randn(4, 1, 8, 8)
        y = torch.randint(0, 2, (4,))
        out = model(x)
        loss = loss_fn(out, y)
        loss.backward()

        for name, p in model.named_parameters():
            assert p.grad is not None, f"{name}.grad is None after backward"
            assert p.grad.abs().max().item() > 0, f"{name}.grad is all-zero after backward"

    @pytest.mark.parametrize("seed", [1, 2, 3, 4])
    def test_gradients_nonzero_multiple_seeds(self, seed: int):
        """Gradient test must hold across seeds (not just one lucky init)."""
        torch.manual_seed(seed * 31)
        model = SmallCNN()
        loss_fn = nn.CrossEntropyLoss()
        x = torch.randn(8, 1, 8, 8)
        y = torch.randint(0, 2, (8,))
        model(x).sum().backward()   # simple proxy loss
        for name, p in model.named_parameters():
            assert p.grad is not None, f"seed={seed}: {name}.grad is None"
            assert p.grad.abs().max() > 0, f"seed={seed}: {name}.grad is zero"

    def test_no_gradient_leakage_between_forward_passes(self):
        """Gradients from one forward should not contaminate the next."""
        torch.manual_seed(70)
        model = SmallCNN()
        loss_fn = nn.CrossEntropyLoss()
        x = torch.randn(4, 1, 8, 8)
        y = torch.randint(0, 2, (4,))

        # First pass
        loss = loss_fn(model(x), y)
        loss.backward()
        grads_first = [p.grad.clone() for p in model.parameters()]

        # Zero + second pass with identical input
        for p in model.parameters():
            p.grad = None
        loss = loss_fn(model(x), y)
        loss.backward()
        grads_second = [p.grad.clone() for p in model.parameters()]

        for g1, g2 in zip(grads_first, grads_second):
            assert torch.allclose(g1, g2, atol=1e-6), (
                "Gradients differ between identical passes — possible accumulation bug"
            )


# ---------------------------------------------------------------------------
# train_step — behaviour and correctness
# ---------------------------------------------------------------------------

class TestTrainStep:
    """train_step must zero gradients, do forward, backward, step, return loss."""

    def test_returns_python_float(self):
        torch.manual_seed(80)
        model = SmallCNN()
        opt = torch.optim.SGD(model.parameters(), lr=0.1)
        X, y = _toy_images(seed=10)
        result = train_step(model, opt, nn.CrossEntropyLoss(), X, y)
        assert type(result) is float, f"Expected float, got {type(result)}"

    def test_returned_loss_is_positive(self):
        """CrossEntropyLoss is always > 0."""
        torch.manual_seed(81)
        model = SmallCNN()
        opt = torch.optim.SGD(model.parameters(), lr=0.1)
        X, y = _toy_images(seed=11)
        val = train_step(model, opt, nn.CrossEntropyLoss(), X, y)
        assert val > 0.0, f"Loss must be positive, got {val}"

    def test_returned_loss_is_finite(self):
        torch.manual_seed(82)
        model = SmallCNN()
        opt = torch.optim.SGD(model.parameters(), lr=0.01)
        X, y = _toy_images(seed=12)
        val = train_step(model, opt, nn.CrossEntropyLoss(), X, y)
        assert math.isfinite(val), f"Loss is not finite: {val}"

    @pytest.mark.parametrize("seed", [0, 5, 9])
    def test_parameters_change_after_step(self, seed: int):
        """Weights must differ from before after train_step (non-zero gradient + lr)."""
        torch.manual_seed(seed)
        model = SmallCNN()
        opt = torch.optim.SGD(model.parameters(), lr=0.1)
        X, y = _toy_images(seed=seed)
        before = [p.data.clone() for p in model.parameters()]
        train_step(model, opt, nn.CrossEntropyLoss(), X, y)
        after = [p.data.clone() for p in model.parameters()]
        assert any(not torch.equal(b, a) for b, a in zip(before, after)), (
            f"seed={seed}: no parameters changed after train_step"
        )

    def test_gradients_zeroed_before_next_step(self):
        """train_step must call zero_grad, not let gradients accumulate.

        Strategy: freeze the weights between two forward-backward passes (no
        optimizer step in the second one) so that the gradient values are
        identical.  If zero_grad is missing the second pass would double them.
        """
        torch.manual_seed(90)
        model = SmallCNN()
        loss_fn = nn.CrossEntropyLoss()
        X, y = _toy_images(seed=13)

        # --- Pass 1: full train_step (optimizer moves weights) ---
        opt = torch.optim.SGD(model.parameters(), lr=0.0)   # lr=0 → weights frozen
        train_step(model, opt, loss_fn, X, y)
        # Grads left by train_step (they were zeroed then filled by backward)
        grads_pass1 = [p.grad.detach().clone() for p in model.parameters()]

        # --- Pass 2: another train_step with frozen weights + same data ---
        # If zero_grad is NOT called inside train_step the grads would double.
        train_step(model, opt, loss_fn, X, y)
        grads_pass2 = [p.grad.detach().clone() for p in model.parameters()]

        for i, (g1, g2) in enumerate(zip(grads_pass1, grads_pass2)):
            assert torch.allclose(g1, g2, atol=1e-5, rtol=1e-4), (
                f"Gradient of param {i} changed between identical passes "
                f"with frozen weights — zero_grad may be missing in train_step"
            )

    def test_loss_decreases_over_many_steps(self):
        """Running train_step repeatedly must drive loss down on the toy dataset."""
        torch.manual_seed(100)
        model = SmallCNN()
        opt = torch.optim.SGD(model.parameters(), lr=0.2)
        loss_fn = nn.CrossEntropyLoss()
        X, y = _toy_images(seed=0)

        losses = [train_step(model, opt, loss_fn, X, y) for _ in range(200)]
        # Loss at the end must be lower than the initial loss
        assert losses[-1] < losses[0], (
            f"Final loss {losses[-1]:.4f} >= initial loss {losses[0]:.4f}"
        )

    def test_loss_much_lower_after_training(self):
        """After sufficient steps the loss must drop to well below half the initial."""
        torch.manual_seed(101)
        model = SmallCNN()
        opt = torch.optim.SGD(model.parameters(), lr=0.2)
        loss_fn = nn.CrossEntropyLoss()
        X, y = _toy_images(seed=1)

        first = train_step(model, opt, loss_fn, X, y)
        for _ in range(149):
            last = train_step(model, opt, loss_fn, X, y)
        assert last < 0.5 * first, (
            f"Final loss {last:.4f} not < 0.5 * initial {first:.4f}"
        )

    @pytest.mark.parametrize("optimizer_cls,lr", [
        (torch.optim.SGD, 0.1),
        (torch.optim.Adam, 1e-3),
    ])
    def test_works_with_multiple_optimizers(self, optimizer_cls, lr):
        """train_step must work regardless of optimizer type."""
        torch.manual_seed(110)
        model = SmallCNN()
        opt = optimizer_cls(model.parameters(), lr=lr)
        X, y = _toy_images(seed=5)
        val = train_step(model, opt, nn.CrossEntropyLoss(), X, y)
        assert isinstance(val, float) and math.isfinite(val)

    def test_single_sample_batch(self):
        """Batch of one must not crash and must return a valid scalar."""
        torch.manual_seed(120)
        model = SmallCNN()
        opt = torch.optim.SGD(model.parameters(), lr=0.1)
        x = torch.randn(1, 1, 8, 8)
        y = torch.tensor([0])
        val = train_step(model, opt, nn.CrossEntropyLoss(), x, y)
        assert isinstance(val, float) and math.isfinite(val) and val > 0


# ---------------------------------------------------------------------------
# Integration: formula consistency with actual CNN architecture
# ---------------------------------------------------------------------------

class TestFormulaArchitectureConsistency:
    """conv_output_size must predict the actual shape that flows through SmallCNN."""

    def test_conv_output_matches_formula(self):
        """After Conv2d(1,4,k=3,stride=1,pad=0) on 8×8, formula and torch agree."""
        torch.manual_seed(200)
        model = SmallCNN()
        x = torch.randn(1, 1, 8, 8)
        with torch.no_grad():
            after_conv = model.conv(x)   # (1, 4, H_out, W_out)
        h_out = after_conv.shape[-1]
        assert h_out == conv_output_size(8, 3, 1, 0), (
            f"Conv output height {h_out} != formula {conv_output_size(8,3,1,0)}"
        )

    def test_pool_output_matches_formula(self):
        """After MaxPool2d(2) on the conv output, formula and torch agree."""
        torch.manual_seed(201)
        model = SmallCNN()
        x = torch.randn(1, 1, 8, 8)
        with torch.no_grad():
            after_pool = model.pool(model.relu(model.conv(x)))
        h_pool = after_pool.shape[-1]
        # Pool is MaxPool2d(2): kernel=2, stride=2, pad=0
        expected = conv_output_size(conv_output_size(8, 3, 1, 0), 2, 2, 0)
        assert h_pool == expected, f"Pool output {h_pool} != formula {expected}"

    def test_flatten_size_matches_linear_in_features(self):
        """The number after flatten must match the Linear layer's in_features."""
        torch.manual_seed(202)
        model = SmallCNN()
        x = torch.randn(2, 1, 8, 8)
        with torch.no_grad():
            after_pool = model.pool(model.relu(model.conv(x)))
        flat_size = after_pool.flatten(1).shape[1]
        assert flat_size == model.fc.in_features, (
            f"Flattened size {flat_size} != fc.in_features {model.fc.in_features}"
        )

    @pytest.mark.parametrize("n", [1, 3, 8])
    def test_end_to_end_shape_various_n(self, n: int):
        """Full forward pass must always yield (N, 2) regardless of batch size."""
        torch.manual_seed(203)
        model = SmallCNN()
        x = torch.randn(n, 1, 8, 8)
        assert tuple(model(x).shape) == (n, 2)


# ---------------------------------------------------------------------------
# receptive_field — formula vs spatial impulse oracle, parametrised
# ---------------------------------------------------------------------------

def _rf_impulse_oracle(num_conv_layers: int, kernel_size: int) -> int:
    """Trace the receptive field with a single-pixel impulse through a
    same-padding stack of all-ones convolutions; sqrt(#nonzero) == RF edge."""
    layers = []
    for _ in range(num_conv_layers):
        conv = nn.Conv2d(
            1, 1, kernel_size=kernel_size, padding=(kernel_size - 1) // 2, bias=True
        )
        with torch.no_grad():
            conv.weight.fill_(1.0)
            conv.bias.zero_()
        layers.append(conv)
    net = nn.Sequential(*layers)
    size = 1 + (kernel_size - 1) * num_conv_layers + 8
    x = torch.zeros(1, 1, size, size)
    x[0, 0, size // 2, size // 2] = 1.0
    with torch.no_grad():
        out = net(x)
    nonzero = int((out.abs() > 1e-8).sum().item())
    return int(round(math.sqrt(nonzero)))


class TestReceptiveField:
    """RF = 1 + (k-1)*depth, verified against a concrete spatial oracle."""

    @pytest.mark.parametrize("n,k", [(1, 3), (2, 3), (3, 3), (4, 3), (1, 5), (2, 5), (3, 5)])
    def test_matches_impulse_oracle(self, n: int, k: int):
        assert receptive_field(n, k) == _rf_impulse_oracle(n, k)

    @pytest.mark.parametrize("k", [3, 5, 7])
    def test_grows_linearly_with_depth(self, k: int):
        """Each extra same-padding conv layer adds exactly (k-1) to the RF edge."""
        prev = receptive_field(1, k)
        for n in range(2, 8):
            cur = receptive_field(n, k)
            assert cur - prev == k - 1
            prev = cur

    @pytest.mark.parametrize("n", [1, 2, 3, 5])
    def test_kernel1_is_pointwise(self, n: int):
        """1×1 convs never widen the receptive field."""
        assert receptive_field(n, 1) == 1

    def test_returns_python_int(self):
        assert type(receptive_field(2, 3)) is int


# ---------------------------------------------------------------------------
# augment_batch — shape / determinism / flip oracle / noise bound invariants
# ---------------------------------------------------------------------------

class TestAugmentBatch:
    """Augmentation must preserve shape, be deterministic per seed, and only
    apply label-preserving transforms (flip / crop+pad / bounded noise)."""

    @pytest.mark.parametrize("n", [1, 2, 5, 16])
    @pytest.mark.parametrize("seed", [0, 7, 99])
    def test_shape_preserved(self, n: int, seed: int):
        torch.manual_seed(seed)
        x = torch.randn(n, 1, 8, 8)
        out = augment_batch(x, seed=seed)
        assert tuple(out.shape) == (n, 1, 8, 8)
        assert out.dtype == torch.float32

    @pytest.mark.parametrize("seed", [0, 1, 5, 13, 100])
    def test_deterministic_given_same_seed(self, seed: int):
        torch.manual_seed(0)
        x = torch.randn(6, 1, 8, 8)
        a = augment_batch(x, seed=seed)
        b = augment_batch(x, seed=seed)
        assert torch.equal(a, b)

    @pytest.mark.parametrize("s1,s2", [(0, 1), (3, 4), (10, 20), (7, 99)])
    def test_different_seeds_give_different_outputs(self, s1: int, s2: int):
        torch.manual_seed(0)
        x = torch.randn(8, 1, 8, 8)
        a = augment_batch(x, seed=s1)
        b = augment_batch(x, seed=s2)
        assert not torch.allclose(a, b)

    @pytest.mark.parametrize("seed", range(8))
    def test_noise_zero_pad_zero_is_identity_or_flip(self, seed: int):
        """With noise_std=0 and pad=0 each image is itself or its mirror — nothing else."""
        torch.manual_seed(0)
        x = torch.randn(6, 1, 8, 8)
        out = augment_batch(x, seed=seed, noise_std=0.0, pad=0)
        for i in range(x.shape[0]):
            same = torch.equal(out[i], x[i])
            flipped = torch.equal(out[i], torch.flip(x[i], dims=[-1]))
            assert same or flipped

    def test_flip_is_involution_oracle(self):
        """Forced-flip path (seed=0, single image) equals torch.flip; flip twice = original."""
        torch.manual_seed(0)
        x = torch.randn(1, 1, 8, 8)
        out = augment_batch(x, seed=0, noise_std=0.0, pad=0)
        assert torch.equal(out, torch.flip(x, dims=[-1]))
        assert torch.equal(torch.flip(out, dims=[-1]), x)

    @pytest.mark.parametrize("noise_std", [0.0, 0.05, 0.1, 0.3])
    def test_noise_magnitude_scales(self, noise_std: float):
        """On a constant image flip/crop are no-ops; mean|out-in| ~ noise_std, ->0 at 0."""
        const = torch.full((4, 1, 8, 8), 0.5)
        out = augment_batch(const, seed=5, noise_std=noise_std, pad=0)
        mad = (out - const).abs().mean().item()
        if noise_std == 0.0:
            assert torch.equal(out, const)
        else:
            assert mad < 3 * noise_std

    @pytest.mark.parametrize("seed", [0, 1, 2])
    def test_finite_on_extreme_inputs(self, seed: int):
        for x in [torch.zeros(3, 1, 8, 8), torch.full((3, 1, 8, 8), 1e4)]:
            out = augment_batch(x, seed=seed)
            assert torch.isfinite(out).all()

    def test_does_not_mutate_input(self):
        torch.manual_seed(0)
        x = torch.randn(4, 1, 8, 8)
        x_before = x.clone()
        _ = augment_batch(x, seed=3)
        assert torch.equal(x, x_before)

    def test_crop_keeps_shape_and_stays_in_range(self):
        """random crop+pad must not change (H, W) and must stay within input range
        (reflect padding never invents out-of-range values; noise off)."""
        torch.manual_seed(0)
        x = torch.randn(5, 1, 8, 8)
        lo, hi = x.min().item(), x.max().item()
        out = augment_batch(x, seed=11, noise_std=0.0, pad=2)
        assert tuple(out.shape) == (5, 1, 8, 8)
        # reflect-pad + crop only reuses existing pixels, so range is preserved
        assert out.min().item() >= lo - 1e-6
        assert out.max().item() <= hi + 1e-6

    def test_signature_is_images_only(self):
        """Augmentation must take/return only images — labels are the caller's job.
        A single tensor in, a single tensor out (documents the label-preserving contract)."""
        torch.manual_seed(0)
        x = torch.randn(4, 1, 8, 8)
        out = augment_batch(x, seed=1)
        assert isinstance(out, torch.Tensor)
        assert out.shape == x.shape
