# Тесты существенного задания: подмодуль mathx.statistics.
#
# Проверяют конкретные значения и краевые случаи.
# Все четыре функции вызываются через публичный API пакета:
#
#     from mathx import mean, median, mode, variance
#
# Это демонстрирует, что реэкспорт в __init__.py работает:
# физически функции лежат в mathx/statistics.py, но снаружи
# они доступны сразу с уровня пакета.

import importlib

import pytest

from mathx import mean, median, mode, variance


# ── mean ──────────────────────────────────────────────────────────────────────

class TestMean:
    def test_mean_integers(self):
        assert mean([1, 2, 3]) == 2.0

    def test_mean_single_element(self):
        assert mean([42]) == 42.0

    def test_mean_floats(self):
        assert mean([0.1, 0.2, 0.3]) == pytest.approx(0.2)

    def test_mean_with_negatives(self):
        assert mean([-3, -1, 1, 3]) == pytest.approx(0.0)

    def test_mean_symmetric_around_zero(self):
        assert mean([-1, 1]) == pytest.approx(0.0)

    def test_mean_all_same(self):
        assert mean([7, 7, 7, 7]) == 7.0

    def test_mean_tuple_input(self):
        # Принимает любой Sequence — кортеж тоже.
        assert mean((10, 20, 30)) == 20.0

    def test_mean_large_values(self):
        assert mean([10 ** 9, 2 * 10 ** 9]) == pytest.approx(1.5 * 10 ** 9)

    def test_mean_empty_raises(self):
        with pytest.raises(ValueError):
            mean([])

    def test_mean_empty_tuple_raises(self):
        with pytest.raises(ValueError):
            mean(())


# ── median ────────────────────────────────────────────────────────────────────

class TestMedian:
    def test_median_odd_sorted(self):
        assert median([1, 2, 3]) == 2.0

    def test_median_odd_unsorted(self):
        assert median([3, 1, 2]) == 2.0

    def test_median_even(self):
        # Чётное: среднее двух центральных
        assert median([1, 2, 3, 4]) == pytest.approx(2.5)

    def test_median_even_non_trivial(self):
        assert median([4, 1, 3, 2]) == pytest.approx(2.5)

    def test_median_single_element(self):
        assert median([7]) == 7.0

    def test_median_two_elements(self):
        assert median([3, 5]) == pytest.approx(4.0)

    def test_median_all_equal(self):
        assert median([9, 9, 9]) == 9.0

    def test_median_with_negatives(self):
        assert median([-5, -1, 0, 3, 10]) == pytest.approx(0.0)

    def test_median_does_not_mutate_input(self):
        data = [3, 1, 2]
        original = list(data)
        median(data)
        assert data == original, "median должна работать с копией, не менять оригинал"

    def test_median_tuple_input(self):
        assert median((5, 1, 3)) == 3.0

    def test_median_empty_raises(self):
        with pytest.raises(ValueError):
            median([])


# ── mode ──────────────────────────────────────────────────────────────────────

class TestMode:
    def test_mode_clear_winner(self):
        assert mode([1, 2, 2, 3]) == 2

    def test_mode_single_element(self):
        assert mode([5]) == 5

    def test_mode_tie_returns_smallest(self):
        # При ничьей — наименьший из победителей
        assert mode([1, 1, 2, 2, 3]) == 1

    def test_mode_all_same(self):
        assert mode([4, 4, 4]) == 4

    def test_mode_triple_tie_returns_smallest(self):
        assert mode([3, 3, 1, 1, 2, 2]) == 1

    def test_mode_all_unique_returns_smallest(self):
        # Все встречаются по разу — тоже ничья, возвращаем наименьший
        assert mode([5, 3, 1]) == 1

    def test_mode_floats(self):
        assert mode([1.5, 2.5, 1.5]) == 1.5

    def test_mode_tuple_input(self):
        assert mode((2, 3, 2, 3, 2)) == 2

    def test_mode_empty_raises(self):
        with pytest.raises(ValueError):
            mode([])


# ── variance ──────────────────────────────────────────────────────────────────

class TestVariance:
    def test_variance_known_value_ddof0(self):
        # Классический пример из учебников: дисперсия = 4.0
        data = [2, 4, 4, 4, 5, 5, 7, 9]
        assert variance(data) == pytest.approx(4.0)

    def test_variance_known_value_ddof1(self):
        data = [2, 4, 4, 4, 5, 5, 7, 9]
        # sum((x - mean)^2) / (8-1): точное значение 32/7
        assert variance(data, ddof=1) == pytest.approx(32.0 / 7.0)

    def test_variance_single_element_ddof0(self):
        assert variance([42.0]) == pytest.approx(0.0)

    def test_variance_all_equal(self):
        # Если все значения одинаковы — дисперсия = 0
        assert variance([3, 3, 3, 3]) == pytest.approx(0.0)

    def test_variance_all_equal_ddof1(self):
        assert variance([3, 3, 3, 3], ddof=1) == pytest.approx(0.0)

    def test_variance_two_elements_ddof0(self):
        # [0, 2]: mean=1, отклонения [-1, 1], сумма квадратов=2, делим на 2 → 1.0
        assert variance([0, 2]) == pytest.approx(1.0)

    def test_variance_two_elements_ddof1(self):
        # [0, 2]: делим на 1 → 2.0
        assert variance([0, 2], ddof=1) == pytest.approx(2.0)

    def test_variance_with_negatives(self):
        # [-1, 1]: mean=0, отклонения [1, 1], сумма=2, ddof=0 → 1.0
        assert variance([-1, 1]) == pytest.approx(1.0)

    def test_variance_tuple_input(self):
        assert variance((1, 2, 3)) == pytest.approx(variance([1, 2, 3]))

    def test_variance_empty_raises(self):
        with pytest.raises(ValueError):
            variance([])

    def test_variance_single_ddof1_raises(self):
        # N-1 = 0 → нельзя
        with pytest.raises(ValueError):
            variance([5], ddof=1)

    def test_variance_invalid_ddof_raises(self):
        with pytest.raises(ValueError):
            variance([1, 2, 3], ddof=2)

    def test_variance_invalid_ddof_negative_raises(self):
        with pytest.raises(ValueError):
            variance([1, 2, 3], ddof=-1)


# ── структура пакета: statistics живёт в mathx ────────────────────────────────

class TestStatisticsSubmodule:
    def test_statistics_submodule_exists(self):
        """mathx.statistics импортируется как подмодуль пакета."""
        stats = importlib.import_module("mathx.statistics")
        assert hasattr(stats, "mean")
        assert hasattr(stats, "median")
        assert hasattr(stats, "mode")
        assert hasattr(stats, "variance")

    def test_statistics_reexported_via_init(self):
        """Имена из statistics доступны напрямую через mathx.*"""
        import mathx
        import mathx.statistics as stats_mod
        assert mathx.mean is stats_mod.mean
        assert mathx.median is stats_mod.median
        assert mathx.mode is stats_mod.mode
        assert mathx.variance is stats_mod.variance

    def test_statistics_in_all(self):
        """mean/median/mode/variance перечислены в mathx.__all__."""
        import mathx
        for name in ("mean", "median", "mode", "variance"):
            assert name in mathx.__all__, f"{name!r} должно быть в mathx.__all__"
