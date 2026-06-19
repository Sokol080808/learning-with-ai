# Тесты для Stack[T] — дженерик-контейнер (существенное задание модуля 13).
# Проверяют КОНКРЕТНЫЕ сценарии: создание, push/pop/peek, len, is_empty, iter, repr.
# Все тесты должны падать на стабе (NotImplementedError) и зеленеть после реализации.

import pytest

from typingtest import Stack


# ─── Создание и is_empty / __len__ ────────────────────────────────────────────

def test_stack_new_is_empty():
    s: Stack[int] = Stack()
    assert s.is_empty() is True


def test_stack_new_len_zero():
    s: Stack[str] = Stack()
    assert len(s) == 0


def test_stack_not_empty_after_push():
    s: Stack[int] = Stack()
    s.push(42)
    assert s.is_empty() is False


def test_stack_len_grows_with_push():
    s: Stack[int] = Stack()
    for i in range(1, 6):
        s.push(i)
        assert len(s) == i


# ─── push / peek / pop — базовый LIFO ─────────────────────────────────────────

def test_stack_peek_returns_last_pushed():
    s: Stack[int] = Stack()
    s.push(10)
    s.push(20)
    assert s.peek() == 20


def test_stack_peek_does_not_change_len():
    s: Stack[int] = Stack()
    s.push(7)
    before = len(s)
    s.peek()
    assert len(s) == before


def test_stack_pop_returns_last_pushed():
    s: Stack[int] = Stack()
    s.push(1)
    s.push(2)
    s.push(3)
    assert s.pop() == 3


def test_stack_pop_decrements_len():
    s: Stack[int] = Stack()
    s.push(1)
    s.push(2)
    s.pop()
    assert len(s) == 1


def test_stack_lifo_order():
    s: Stack[int] = Stack()
    items = [10, 20, 30, 40]
    for x in items:
        s.push(x)
    result = []
    while not s.is_empty():
        result.append(s.pop())
    assert result == list(reversed(items))


def test_stack_push_then_pop_restores_empty():
    s: Stack[str] = Stack()
    s.push("hello")
    s.pop()
    assert s.is_empty()
    assert len(s) == 0


# ─── Ошибки на пустом стеке ────────────────────────────────────────────────────

def test_stack_pop_empty_raises_index_error():
    s: Stack[int] = Stack()
    with pytest.raises(IndexError):
        s.pop()


def test_stack_peek_empty_raises_index_error():
    s: Stack[float] = Stack()
    with pytest.raises(IndexError):
        s.peek()


def test_stack_pop_after_drain_raises_index_error():
    s: Stack[int] = Stack()
    s.push(1)
    s.pop()
    with pytest.raises(IndexError):
        s.pop()


# ─── Разные типы элементов ─────────────────────────────────────────────────────

def test_stack_str_elements():
    s: Stack[str] = Stack()
    s.push("alpha")
    s.push("beta")
    assert s.pop() == "beta"
    assert s.pop() == "alpha"


def test_stack_tuple_elements():
    s: Stack[tuple[int, int]] = Stack()
    s.push((1, 2))
    s.push((3, 4))
    assert s.peek() == (3, 4)


def test_stack_nested_stack():
    # стек стеков — убеждаемся, что Generic не конфликтует с вложением
    inner: Stack[int] = Stack()
    inner.push(99)
    outer: Stack[Stack[int]] = Stack()
    outer.push(inner)
    assert len(outer) == 1


# ─── __iter__ ─────────────────────────────────────────────────────────────────

def test_stack_iter_order_top_to_bottom():
    s: Stack[int] = Stack()
    for x in [1, 2, 3]:
        s.push(x)
    # итерация: 3 (вершина) → 2 → 1 (основание)
    assert list(s) == [3, 2, 1]


def test_stack_iter_does_not_mutate():
    s: Stack[int] = Stack()
    s.push(10)
    s.push(20)
    before_len = len(s)
    _ = list(s)
    assert len(s) == before_len
    assert s.peek() == 20  # вершина на месте


def test_stack_iter_empty_is_empty_sequence():
    s: Stack[int] = Stack()
    assert list(s) == []


def test_stack_iter_twice_same_result():
    s: Stack[int] = Stack()
    s.push(1)
    s.push(2)
    assert list(s) == list(s)


# ─── __repr__ ─────────────────────────────────────────────────────────────────

def test_stack_repr_is_string():
    s: Stack[int] = Stack()
    assert isinstance(repr(s), str)


def test_stack_repr_not_empty():
    s: Stack[int] = Stack()
    s.push(1)
    assert repr(s) != ""


# ─── Независимость двух стеков ────────────────────────────────────────────────

def test_stack_two_instances_independent():
    a: Stack[int] = Stack()
    b: Stack[int] = Stack()
    a.push(1)
    b.push(2)
    assert a.pop() == 1
    assert b.pop() == 2


def test_stack_push_to_one_does_not_affect_other():
    a: Stack[int] = Stack()
    b: Stack[int] = Stack()
    a.push(100)
    assert len(b) == 0
    assert b.is_empty()
