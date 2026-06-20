# Тесты для задания «четыре кита»: mse_loss.
#
# Файл трогать не нужно — это эталон поведения.
# На ветке main warmup.mse_loss бросает NotImplementedError, поэтому
# все тесты здесь падают (красные) до тех пор, пока ты не реализуешь функцию.
#
# Структура:
#   TestMseLossExamples  — детерминированные численные примеры, руками проверяемые.
#   TestMseLossTorchOracle — сравнение с torch.nn.functional.mse_loss на случайных
#                            входах (фиксированные сиды для воспроизводимости).
#   TestMseLossProperties  — инвариантные свойства (симметрия, неотрицательность,
#                            масштабирование, произвольные формы тензоров).

import numpy as np
import pytest
import torch
import torch.nn.functional as F

from warmup import mse_loss


# ---------------------------------------------------------------------------
# Детерминированные примеры
# ---------------------------------------------------------------------------

class TestMseLossExamples:
    """Руками вычисленные случаи — нулевая ошибка, единичное отклонение, батч."""

    def test_identical_arrays_zero_loss(self):
        """Если pred == target, потеря ровно 0."""
        pred = np.array([1.0, 2.0, 3.0])
        target = np.array([1.0, 2.0, 3.0])
        assert mse_loss(pred, target) == 0.0

    def test_single_element_unit_error(self):
        """mse_loss([0.], [1.]) = (0-1)^2 / 1 = 1.0."""
        assert mse_loss(np.array([0.0]), np.array([1.0])) == pytest.approx(1.0)

    def test_two_element_mixed(self):
        """mse_loss([1, 2], [3, 2]) = ((1-3)^2 + (2-2)^2) / 2 = 4/2 = 2.0."""
        pred = np.array([1.0, 2.0])
        target = np.array([3.0, 2.0])
        assert mse_loss(pred, target) == pytest.approx(2.0)

    def test_four_element_known(self):
        """mse_loss([0,0,0,0], [1,2,3,4]) = (1+4+9+16)/4 = 7.5."""
        pred = np.zeros(4)
        target = np.array([1.0, 2.0, 3.0, 4.0])
        assert mse_loss(pred, target) == pytest.approx(7.5)

    def test_constant_offset(self):
        """Все предсказания смещены на одно и то же число c.
        loss = c^2.
        """
        c = 3.0
        n = 10
        pred = np.full(n, c)
        target = np.zeros(n)
        assert mse_loss(pred, target) == pytest.approx(c ** 2)

    def test_returns_python_float(self):
        """Контракт: возвращаемый тип — именно питоновский float."""
        pred = np.array([1.0, 2.0])
        target = np.array([1.5, 2.5])
        result = mse_loss(pred, target)
        assert isinstance(result, float), (
            f"ожидается float, получен {type(result)}"
        )

    def test_result_is_nonnegative(self):
        """MSE неотрицательна по определению (среднее квадратов)."""
        np.random.seed(0)
        pred = np.random.randn(20)
        target = np.random.randn(20)
        assert mse_loss(pred, target) >= 0.0

    def test_2d_matrix_shape(self):
        """Работает на матрице (N, D): форма входа не должна влиять на результат.

        mse_loss([[0,0],[0,0]], [[1,2],[3,4]]) = (1+4+9+16)/4 = 7.5.
        """
        pred = np.zeros((2, 2))
        target = np.array([[1.0, 2.0], [3.0, 4.0]])
        assert mse_loss(pred, target) == pytest.approx(7.5)

    def test_rank3_tensor(self):
        """Тензор ранга 3 (B, H, W) — все разности равны 1.
        loss = mean(1^2) = 1.0.
        """
        pred = np.zeros((2, 3, 4))
        target = np.ones((2, 3, 4))
        assert mse_loss(pred, target) == pytest.approx(1.0)


# ---------------------------------------------------------------------------
# Torch-оракул на случайных входах
# ---------------------------------------------------------------------------

class TestMseLossTorchOracle:
    """Сравниваем наш mse_loss с torch.nn.functional.mse_loss (reduction='mean')."""

    @staticmethod
    def _torch_mse(pred: np.ndarray, target: np.ndarray) -> float:
        tp = torch.from_numpy(pred.astype(np.float64))
        tt = torch.from_numpy(target.astype(np.float64))
        return F.mse_loss(tp, tt, reduction="mean").item()

    def test_1d_random_float64(self):
        np.random.seed(100)
        for _ in range(20):
            n = np.random.randint(1, 200)
            pred = np.random.randn(n)
            target = np.random.randn(n)
            ours = mse_loss(pred, target)
            ref = self._torch_mse(pred, target)
            assert abs(ours - ref) < 1e-10, (
                f"1-D n={n}: наш={ours:.6g} torch={ref:.6g}"
            )

    def test_2d_random_float64(self):
        np.random.seed(101)
        for shape in [(4, 5), (10, 3), (1, 100)]:
            pred = np.random.randn(*shape)
            target = np.random.randn(*shape)
            ours = mse_loss(pred, target)
            ref = self._torch_mse(pred, target)
            assert abs(ours - ref) < 1e-10, (
                f"2-D shape={shape}: наш={ours:.6g} torch={ref:.6g}"
            )

    def test_3d_random_float64(self):
        """Тензор ранга 3 — «батч картинок» (B, H, W)."""
        np.random.seed(102)
        for shape in [(2, 8, 8), (4, 4, 4), (1, 1, 50)]:
            pred = np.random.randn(*shape)
            target = np.random.randn(*shape)
            ours = mse_loss(pred, target)
            ref = self._torch_mse(pred, target)
            assert abs(ours - ref) < 1e-10, (
                f"3-D shape={shape}: наш={ours:.6g} torch={ref:.6g}"
            )

    def test_float32_tolerance(self):
        """float32: torch и numpy могут чуть разойтись из-за округления."""
        np.random.seed(103)
        for _ in range(15):
            n = np.random.randint(1, 300)
            pred = np.random.randn(n).astype(np.float32)
            target = np.random.randn(n).astype(np.float32)
            ours = mse_loss(pred, target)
            ref = self._torch_mse(pred.astype(np.float64),
                                   target.astype(np.float64))
            # допуск чуть шире из-за float32 → float64 конвертации
            assert abs(ours - ref) < 1e-4, (
                f"float32 n={n}: наш={ours:.6g} ref={ref:.6g}"
            )


# ---------------------------------------------------------------------------
# Инвариантные свойства
# ---------------------------------------------------------------------------

class TestMseLossProperties:
    """Математические свойства MSE, не зависящие от конкретных значений."""

    def test_symmetry(self):
        """mse_loss(pred, target) == mse_loss(target, pred).
        (pred - target)^2 == (target - pred)^2 поэлементно.
        """
        np.random.seed(200)
        for _ in range(15):
            n = np.random.randint(2, 100)
            pred = np.random.randn(n)
            target = np.random.randn(n)
            assert abs(mse_loss(pred, target) - mse_loss(target, pred)) < 1e-13

    def test_nonnegative_always(self):
        """MSE >= 0 при любых входах."""
        np.random.seed(201)
        for _ in range(20):
            shape = tuple(np.random.randint(1, 10, size=np.random.randint(1, 4)))
            pred = np.random.randn(*shape)
            target = np.random.randn(*shape)
            assert mse_loss(pred, target) >= 0.0

    def test_scale_pred(self):
        """Масштабирование pred: mse_loss(c*pred, target) != mse_loss(pred, target) (обычно).
        Конкретно: mse_loss(2*a, zeros) = 4 * mse_loss(a, zeros).
        """
        np.random.seed(202)
        a = np.random.randn(50)
        zeros = np.zeros(50)
        loss1 = mse_loss(a, zeros)
        loss2 = mse_loss(2 * a, zeros)
        assert abs(loss2 - 4 * loss1) < 1e-10

    def test_shift_invariance_both(self):
        """Сдвиг pred и target на одно и то же не меняет loss:
        mse_loss(pred + c, target + c) == mse_loss(pred, target).
        """
        np.random.seed(203)
        for _ in range(15):
            n = np.random.randint(2, 80)
            pred = np.random.randn(n)
            target = np.random.randn(n)
            c = float(np.random.randn())
            assert abs(
                mse_loss(pred + c, target + c) - mse_loss(pred, target)
            ) < 1e-10, f"shift invariance failed for c={c}"

    def test_zero_loss_iff_equal(self):
        """loss == 0 тогда и только тогда, когда pred == target поэлементно."""
        np.random.seed(204)
        for _ in range(10):
            n = np.random.randint(1, 50)
            a = np.random.randn(n)
            assert mse_loss(a, a) == 0.0

    def test_single_element_equals_squared_diff(self):
        """Для одного элемента loss = (pred - target)^2."""
        np.random.seed(205)
        for _ in range(15):
            p = float(np.random.randn())
            t = float(np.random.randn())
            expected = (p - t) ** 2
            result = mse_loss(np.array([p]), np.array([t]))
            assert abs(result - expected) < 1e-12

    def test_large_magnitudes_finite(self):
        """При больших значениях результат конечен."""
        np.random.seed(206)
        pred = np.random.randn(100) * 1e7
        target = np.random.randn(100) * 1e7
        result = mse_loss(pred, target)
        assert np.isfinite(result), f"mse_loss вернул не-конечное: {result}"

    def test_works_on_arbitrary_rank3(self):
        """Тензор формы (B, N, D) — «batched data»: результат float, не массив."""
        np.random.seed(207)
        pred = np.random.randn(3, 5, 7)
        target = np.random.randn(3, 5, 7)
        result = mse_loss(pred, target)
        assert isinstance(result, float)
        assert result >= 0.0
