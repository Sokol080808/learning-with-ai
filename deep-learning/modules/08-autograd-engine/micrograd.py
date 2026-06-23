# ВНИМАНИЕ: здесь пишешь ТЫ. Реализуй так, чтобы тесты модуля 08 стали зелёными.
#
# Это скелет учебного движка автодифференцирования (свой маленький "micrograd").
# Заполни тела методов. Готового решения тут нет — только контракт в docstring.
from __future__ import annotations

import math
import random
from typing import Callable, Iterable, List, Set, Tuple, Union

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
        raise NotImplementedError("TODO: сохрани data/grad/_prev/_op/_backward")

    def __add__(self, other: Union["Value", Number]) -> "Value":
        """Сложение: out = self + other. Вернуть НОВЫЙ Value.

        Если other — обычное число, заверни его в Value(other).
        Локальная производная сложения: d(out)/d(self) = 1, d(out)/d(other) = 1.
        Поэтому _backward должен делать:
            self.grad  += 1.0 * out.grad
            other.grad += 1.0 * out.grad
        """
        raise NotImplementedError("TODO: реализуй __add__ и его _backward")

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
        raise NotImplementedError("TODO: реализуй __mul__ и его _backward")

    def tanh(self) -> "Value":
        """Гиперболический тангенс: out = tanh(self). Вернуть НОВЫЙ Value.

        Значение: t = tanh(self.data)  (можно через math.tanh).
        Производная: d(tanh)/dx = 1 - tanh(x)^2 = 1 - t^2.
        Поэтому _backward:
            self.grad += (1 - t**2) * out.grad
        """
        raise NotImplementedError("TODO: реализуй tanh и его _backward")

    def relu(self) -> "Value":
        """ReLU: out = max(0, self). Вернуть НОВЫЙ Value.

        Значение: 0.0 если self.data < 0, иначе self.data.
        Производная: 0 при x < 0, 1 при x > 0 (в нуле берём 0 — это договорённость).
        Удобный способ: множитель = 1.0 если out.data > 0 иначе 0.0.
        Поэтому _backward:
            self.grad += (1.0 if out.data > 0 else 0.0) * out.grad
        """
        raise NotImplementedError("TODO: реализуй relu и его _backward")

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
        raise NotImplementedError("TODO: топологическая сортировка + обратный проход")

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


# =========================================================================== #
# Задание 7 — сеть поверх Value: Neuron → Layer → MLP.
#
# Это «вершина» лекции Карпатого про micrograd: сам движок (Value) уже готов,
# и теперь мы строим на нём НАСТОЯЩУЮ нейросеть. Ничего нового в автограде не
# нужно — Neuron/Layer/MLP это просто УДОБНЫЕ КОНТЕЙНЕРЫ из Value-узлов.
# Forward строит граф, .backward() от loss заполняет .grad у всех весов,
# и тот же цикл «обнули .grad → backward → шаг по -grad», что и в одном нейроне,
# обучает уже многослойную сеть.
# =========================================================================== #


class Module:
    """Общий родитель Neuron/Layer/MLP: умеет обнулять и перечислять параметры.

    parameters() возвращает ПЛОСКИЙ список всех обучаемых Value (веса + смещения).
    zero_grad() ставит .grad = 0.0 каждому параметру — это и есть ручной аналог
    optimizer.zero_grad() из PyTorch. В цикле обучения его вызывают ПЕРЕД backward(),
    иначе градиенты с прошлых шагов накопятся (backward делает +=, а не =).
    """

    def parameters(self) -> List["Value"]:
        raise NotImplementedError("TODO: вернуть плоский список всех Value-параметров")

    def zero_grad(self) -> None:
        raise NotImplementedError("TODO: обнулить .grad у каждого параметра")


class Neuron(Module):
    """Один нейрон: out = activation( w·x + b ).

    Хранит:
      - w : list[Value]   — по одному весу на каждый вход (n_in штук);
      - b : Value         — смещение (bias), один на нейрон;
      - nonlin : str      — 'tanh', 'relu' или 'linear' (без активации).

    __call__(x) принимает список входов (числа или Value), считает взвешенную
    сумму  act = b + Σ w_i * x_i,  затем применяет нелинейность и возвращает Value.

    Веса инициализируются маленькими случайными числами; если передан rng
    (random.Random), берём числа из него — так тесты делаются детерминированными.
    """

    def __init__(
        self,
        n_in: int,
        nonlin: str = "tanh",
        rng: random.Random | None = None,
    ) -> None:
        raise NotImplementedError("TODO: создать self.w (список Value), self.b (Value), self.nonlin")

    def __call__(self, x: Iterable[Union["Value", Number]]) -> "Value":
        raise NotImplementedError("TODO: посчитать взвешенную сумму + применить нелинейность")

    def parameters(self) -> List["Value"]:
        raise NotImplementedError("TODO: вернуть self.w + [self.b]")

    def __repr__(self) -> str:
        raise NotImplementedError("TODO: вернуть строку вида 'TanhNeuron(n_in=...)'")


class Layer(Module):
    """Слой = список из n_out независимых нейронов, каждый видит все n_in входов.

    __call__(x) возвращает СПИСОК из n_out Value (выход каждого нейрона).
    Параметры слоя — это все параметры всех его нейронов, сплющенные в один список.
    """

    def __init__(
        self,
        n_in: int,
        n_out: int,
        nonlin: str = "tanh",
        rng: random.Random | None = None,
    ) -> None:
        raise NotImplementedError("TODO: создать self.neurons — список из n_out Neuron'ов")

    def __call__(self, x: Iterable[Union["Value", Number]]) -> List["Value"]:
        raise NotImplementedError("TODO: прогнать x через каждый нейрон, вернуть список")

    def parameters(self) -> List["Value"]:
        raise NotImplementedError("TODO: собрать параметры всех нейронов в плоский список")

    def __repr__(self) -> str:
        raise NotImplementedError("TODO: вернуть строку вида 'Layer of [...]'")


class MLP(Module):
    """Многослойный перцептрон: цепочка Layer'ов.

    layer_sizes = [n_in, h1, h2, ..., n_out] задаёт размеры:
      первый элемент — число входов, остальные — ширины слоёв.
    Например MLP([2, 3, 1]) — вход 2 признака, скрытый слой из 3 нейронов, выход 1.

    Скрытые слои получают активацию nonlin (по умолчанию 'tanh'); ПОСЛЕДНИЙ слой
    делается линейным ('linear') — типичный приём, чтобы выход не зажимался tanh'ом
    в (-1, 1). Это удобный дефолт; при желании можно собрать слои вручную.

    __call__(x): прогоняем x через все слои по очереди. Если на выходе ровно один
    Value (n_out == 1), возвращаем его одного, а не список из одного элемента —
    так с одиночным выходом удобнее работать (loss = (pred - y)**2).
    """

    def __init__(
        self,
        layer_sizes: List[int],
        nonlin: str = "tanh",
        rng: random.Random | None = None,
    ) -> None:
        raise NotImplementedError("TODO: создать self.layers — список Layer'ов по layer_sizes")

    def __call__(
        self, x: Iterable[Union["Value", Number]]
    ) -> Union["Value", List["Value"]]:
        raise NotImplementedError("TODO: прогнать x через все слои; если выход один — вернуть Value, иначе список")

    def parameters(self) -> List["Value"]:
        raise NotImplementedError("TODO: собрать параметры всех слоёв в плоский список")

    def __repr__(self) -> str:
        raise NotImplementedError("TODO: вернуть строку вида 'MLP of [...]'")
