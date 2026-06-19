# Эти тесты трогать не нужно — это эталон поведения.
#
# Они проверяют твой скалярный autograd (класс Value):
#   * forward даёт правильное число,
#   * backward() заполняет .grad всех узлов,
#   * градиенты совпадают с аналитическими И с численной производной,
#   * накопление градиента при повторном использовании узла,
#   * крошечный шаг обучения уменьшает loss.
import math

import numpy as np

from micrograd import Value


# --------------------------------------------------------------------------- #
# Вспомогательное: численная производная f по одному скалярному входу.
# Возвращает (f(x+h) - f(x-h)) / (2h) — центральная разность, точная до O(h^2).
# Так мы независимо от реализации Value получаем эталон для сравнения с .grad.
# --------------------------------------------------------------------------- #
def numeric_grad(f, x, h=1e-6):
    return (f(x + h) - f(x - h)) / (2 * h)


def test_forward_values():
    # Простейшая арифметика: forward должен совпадать с обычными float.
    a = Value(2.0)
    b = Value(-3.0)
    c = Value(10.0)
    out = a * b + c  # 2*(-3) + 10 = 4
    assert math.isclose(out.data, 4.0, rel_tol=1e-9)

    assert math.isclose((Value(3.0) + Value(4.0)).data, 7.0, rel_tol=1e-9)
    assert math.isclose((Value(3.0) * Value(4.0)).data, 12.0, rel_tol=1e-9)
    assert math.isclose(Value(0.5).tanh().data, math.tanh(0.5), rel_tol=1e-9)
    assert math.isclose(Value(-2.0).relu().data, 0.0, abs_tol=1e-12)
    assert math.isclose(Value(2.0).relu().data, 2.0, rel_tol=1e-9)


def test_scalars_promoted():
    # Числа должны автоматически заворачиваться в Value: 2 * a, a + 1, и т.п.
    a = Value(3.0)
    assert math.isclose((a + 1).data, 4.0, rel_tol=1e-9)
    assert math.isclose((1 + a).data, 4.0, rel_tol=1e-9)
    assert math.isclose((2 * a).data, 6.0, rel_tol=1e-9)
    assert math.isclose((a * 2).data, 6.0, rel_tol=1e-9)
    assert math.isclose((a - 1).data, 2.0, rel_tol=1e-9)
    assert math.isclose((5 - a).data, 2.0, rel_tol=1e-9)
    assert math.isclose((-a).data, -3.0, rel_tol=1e-9)


def test_grad_zero_before_backward():
    # До вызова backward() градиент — ровно 0.0.
    a = Value(1.5)
    b = Value(2.5)
    out = a * b
    assert a.grad == 0.0
    assert b.grad == 0.0
    assert out.grad == 0.0


def test_backward_flagship_expression():
    # ГЛАВНЫЙ тест из спецификации: ((a*b)+c).tanh().
    a = Value(2.0)
    b = Value(-3.0)
    c = Value(10.0)
    out = (a * b + c).tanh()
    out.backward()

    # Аналитика. Пусть s = a*b + c, out = tanh(s), g = 1 - tanh(s)^2.
    s = 2.0 * (-3.0) + 10.0
    g = 1.0 - math.tanh(s) ** 2
    # d(out)/da = g * b, d(out)/db = g * a, d(out)/dc = g.
    assert np.allclose(a.grad, g * (-3.0), atol=1e-6)
    assert np.allclose(b.grad, g * 2.0, atol=1e-6)
    assert np.allclose(c.grad, g, atol=1e-6)
    # И производная результата по самому себе равна 1.
    assert np.allclose(out.grad, 1.0, atol=1e-12)


def test_backward_matches_numeric_gradient():
    # Сверяем .grad с НЕЗАВИСИМОЙ численной производной той же функции.
    a0, b0, c0 = 1.3, -0.7, 0.4

    def expr(av, bv, cv):
        return (Value(av) * Value(bv) + Value(cv)).tanh().data

    a = Value(a0)
    b = Value(b0)
    c = Value(c0)
    out = (a * b + c).tanh()
    out.backward()

    da = numeric_grad(lambda x: expr(x, b0, c0), a0)
    db = numeric_grad(lambda x: expr(a0, x, c0), b0)
    dc = numeric_grad(lambda x: expr(a0, b0, x), c0)

    assert np.allclose(a.grad, da, atol=1e-5)
    assert np.allclose(b.grad, db, atol=1e-5)
    assert np.allclose(c.grad, dc, atol=1e-5)


def test_relu_gradient_both_sides():
    # ReLU: при x>0 градиент проходит как есть, при x<0 — обнуляется.
    xpos = Value(2.0)
    ypos = (xpos * 3.0).relu()  # 6 > 0 -> производная по xpos = 3
    ypos.backward()
    assert np.allclose(xpos.grad, 3.0, atol=1e-9)

    xneg = Value(-2.0)
    yneg = (xneg * 3.0).relu()  # -6 < 0 -> производная = 0
    yneg.backward()
    assert np.allclose(xneg.grad, 0.0, atol=1e-9)


def test_grad_accumulates_when_node_reused():
    # Узел, использованный дважды, должен НАКАПЛИВАТЬ градиент (+=), а не перезаписывать.
    # f = x * x  =>  df/dx = 2x. При x=3 ожидаем 6 (3 от одной ветви + 3 от другой).
    x = Value(3.0)
    y = x * x
    y.backward()
    assert np.allclose(x.grad, 6.0, atol=1e-9)

    # f = a + a => df/da = 2.
    a = Value(5.0)
    z = a + a
    z.backward()
    assert np.allclose(a.grad, 2.0, atol=1e-9)


def test_deeper_graph_numeric():
    # Более глубокое выражение: out = tanh(a*b + relu(c)) * a + b.
    a0, b0, c0 = 0.5, -1.2, 0.9

    def expr(av, bv, cv):
        a, b, c = Value(av), Value(bv), Value(cv)
        out = (a * b + c.relu()).tanh() * a + b
        return out

    a = Value(a0)
    b = Value(b0)
    c = Value(c0)
    out = (a * b + c.relu()).tanh() * a + b
    out.backward()

    def fval(av, bv, cv):
        return expr(av, bv, cv).data

    da = numeric_grad(lambda x: fval(x, b0, c0), a0)
    db = numeric_grad(lambda x: fval(a0, x, c0), b0)
    dc = numeric_grad(lambda x: fval(a0, b0, x), c0)

    assert np.allclose(a.grad, da, atol=1e-5)
    assert np.allclose(b.grad, db, atol=1e-5)
    assert np.allclose(c.grad, dc, atol=1e-5)


def test_training_reduces_loss():
    # Учим один скалярный "нейрон" w*x + b приближать целевое значение.
    # Это проверяет, что .grad годятся для градиентного спуска: loss должен заметно упасть.
    np.random.seed(0)
    x_val = 1.5
    target = 0.8

    w = Value(np.random.randn() * 0.1)
    b = Value(np.random.randn() * 0.1)

    def forward():
        # loss = (tanh(w*x + b) - target)^2
        pred = (w * x_val + b).tanh()
        diff = pred - target
        return diff * diff

    loss0 = forward().data

    lr = 0.5
    for _ in range(200):
        loss = forward()
        # обнуляем градиенты перед каждым backward
        w.grad = 0.0
        b.grad = 0.0
        loss.backward()
        # шаг градиентного спуска
        w.data -= lr * w.grad
        b.data -= lr * b.grad

    loss_final = forward().data
    assert loss_final < 0.5 * loss0
