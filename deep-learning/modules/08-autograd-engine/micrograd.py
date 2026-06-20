# ВНИМАНИЕ: здесь пишешь ТЫ. Реализуй так, чтобы тесты модуля 08 стали зелёными.
#
# Это скелет учебного движка автодифференцирования (свой маленький "micrograd").
# Заполни тела методов. Готового решения тут нет — только контракт в docstring.
from __future__ import annotations

import math
from typing import Callable, Iterable, Set, Tuple, Union

# Чему разрешаем участвовать в арифметике с Value: либо другой Value, либо обычное число.
Number = Union[int, float]


class Value:
    """Скалярный узел графа вычислений (один-единственный float + его градиент).

    Каждый Value хранит:
      - data : float                — само число (результат вычисления в этом узле);
      - grad : float                — производная итогового результата по этому узлу,
                                      d(out)/d(self). До backward() градиент равен 0.0;
      - _prev: set[Value]           — родители: узлы, ИЗ которых получился этот;
      - _backward: Callable[[], None] — локальный шаг обратного прохода: берёт уже
                                      посчитанный self.grad и ДОБАВЛЯЕТ (`+=`) вклад
                                      в .grad каждого родителя по цепному правилу.

    Идея: пока ты строишь выражение (a*b + c).tanh(), узлы Value сами запоминают,
    из кого они получены и как "протолкнуть" градиент назад. Метод backward() потом
    обходит этот граф в правильном порядке и заполняет .grad у всех узлов.

    Накопление (+=) важно: если один узел используется несколько раз
    (например x*x или a + a), его вклады в градиент по разным ветвям СУММИРУЮТСЯ.
    """

    def __init__(
        self,
        data: Number,
        _children: Tuple["Value", ...] = (),
        _op: str = "",
    ) -> None:
        """Создать узел.

        Аргументы:
          data      — число, которое хранит узел.
          _children — узлы-родители (служебное; ты передаёшь их сам внутри операций).
          _op       — короткая метка операции ('+', '*', 'tanh', ...); только для отладки.

        Должно быть выставлено:
          self.data = float(data)
          self.grad = 0.0
          self._prev = set(_children)
          self._op = _op
          self._backward = lambda: None   # по умолчанию — ничего (для листьев графа)
        """
        self.data = float(data)
        self.grad = 0.0
        self._prev: Set["Value"] = set(_children)
        self._op = _op
        self._backward: Callable[[], None] = lambda: None

    def __add__(self, other: Union["Value", Number]) -> "Value":
        """Сложение: out = self + other. Вернуть НОВЫЙ Value.

        Если other — обычное число, заверни его в Value(other).
        Локальная производная сложения: d(out)/d(self) = 1, d(out)/d(other) = 1.
        Поэтому _backward должен делать:
            self.grad  += 1.0 * out.grad
            other.grad += 1.0 * out.grad
        """
        other = other if isinstance(other, Value) else Value(other)
        out = Value(self.data + other.data, (self, other), "+")

        def _backward() -> None:
            self.grad += 1.0 * out.grad
            other.grad += 1.0 * out.grad

        out._backward = _backward
        return out

    def __mul__(self, other: Union["Value", Number]) -> "Value":
        """Умножение: out = self * other. Вернуть НОВЫЙ Value.

        Если other — число, заверни его в Value(other).
        Локальная производная умножения:
            d(out)/d(self)  = other.data
            d(out)/d(other) = self.data
        Поэтому _backward:
            self.grad  += other.data * out.grad
            other.grad += self.data  * out.grad
        """
        other = other if isinstance(other, Value) else Value(other)
        out = Value(self.data * other.data, (self, other), "*")

        def _backward() -> None:
            self.grad += other.data * out.grad
            other.grad += self.data * out.grad

        out._backward = _backward
        return out

    def tanh(self) -> "Value":
        """Гиперболический тангенс: out = tanh(self). Вернуть НОВЫЙ Value.

        Значение: t = tanh(self.data)  (можно через math.tanh).
        Производная: d(tanh)/dx = 1 - tanh(x)^2 = 1 - t^2.
        Поэтому _backward:
            self.grad += (1 - t**2) * out.grad
        """
        t = math.tanh(self.data)
        out = Value(t, (self,), "tanh")

        def _backward() -> None:
            self.grad += (1 - t**2) * out.grad

        out._backward = _backward
        return out

    def relu(self) -> "Value":
        """ReLU: out = max(0, self). Вернуть НОВЫЙ Value.

        Значение: 0.0 если self.data < 0, иначе self.data.
        Производная: 0 при x < 0, 1 при x > 0 (в нуле берём 0 — это договорённость).
        Удобный способ: множитель = 1.0 если out.data > 0 иначе 0.0.
        Поэтому _backward:
            self.grad += (1.0 if out.data > 0 else 0.0) * out.grad
        """
        out = Value(self.data if self.data > 0 else 0.0, (self,), "relu")

        def _backward() -> None:
            self.grad += (1.0 if out.data > 0 else 0.0) * out.grad

        out._backward = _backward
        return out

    def backward(self) -> None:
        """Запустить обратный проход ОТ ЭТОГО узла (обычно от loss).

        Шаги:
          1) Построить топологический порядок узлов графа (через DFS из self):
             узел добавляется в список ПОСЛЕ всех своих родителей (_prev).
          2) Выставить self.grad = 1.0  (производная результата по самому себе).
          3) Пройти список в ОБРАТНОМ порядке и в каждом узле вызвать node._backward().
             Так к моменту обработки узла его .grad уже полностью накоплен, и можно
             корректно протолкнуть вклад дальше к родителям.

        После вызова у каждого узла графа поле .grad содержит d(self)/d(этот узел).
        Замечание: метод НИЧЕГО не возвращает — он только заполняет .grad на месте.
        """
        topo: list["Value"] = []
        visited: Set["Value"] = set()

        def build(v: "Value") -> None:
            if v not in visited:
                visited.add(v)
                for parent in v._prev:
                    build(parent)
                topo.append(v)

        build(self)

        self.grad = 1.0
        for node in reversed(topo):
            node._backward()

    # --- Удобные обёртки: чтобы работали выражения вида (2 * a), (b - 1), (-x). ---
    # Эти методы УЖЕ написаны через __add__/__mul__ — отдельные _backward им не нужны.

    def __neg__(self) -> "Value":  # -self
        return self * -1

    def __sub__(self, other: Union["Value", Number]) -> "Value":  # self - other
        return self + (-other if isinstance(other, Value) else Value(other) * -1)

    def __radd__(self, other: Union["Value", Number]) -> "Value":  # other + self
        return self + other

    def __rmul__(self, other: Union["Value", Number]) -> "Value":  # other * self
        return self * other

    def __rsub__(self, other: Union["Value", Number]) -> "Value":  # other - self
        return Value(other) + (-self)

    def __repr__(self) -> str:
        return f"Value(data={self.data}, grad={self.grad})"
