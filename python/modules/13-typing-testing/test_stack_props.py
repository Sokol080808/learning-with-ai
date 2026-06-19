# Property-тесты для Stack[T] (hypothesis, derandomize=True через conftest.py).
# Проверяют ИНВАРИАНТЫ, которые обязаны выполняться для ЛЮБОГО входа:
#   - round-trip push/pop,
#   - размер и границы,
#   - порядок обхода,
#   - идемпотентность наблюдений,
#   - независимость копий.

import pytest
from hypothesis import given, settings
from hypothesis import strategies as st

from typingtest import Stack

# Стратегии для элементов
ints = st.integers(min_value=-(10**9), max_value=10**9)
strs = st.text(min_size=0, max_size=20)
small_lists = st.lists(ints, min_size=0, max_size=50)
nonempty_lists = st.lists(ints, min_size=1, max_size=50)


# ─── Размер ───────────────────────────────────────────────────────────────────

@given(xs=small_lists)
def test_stack_prop_len_equals_pushes(xs: list[int]) -> None:
    """len() должен совпадать с числом push."""
    s: Stack[int] = Stack()
    for i, x in enumerate(xs):
        s.push(x)
        assert len(s) == i + 1


@given(xs=nonempty_lists)
def test_stack_prop_pop_decrements_len(xs: list[int]) -> None:
    """Каждый pop уменьшает len на 1."""
    s: Stack[int] = Stack()
    for x in xs:
        s.push(x)
    n = len(xs)
    for expected_len in range(n - 1, -1, -1):
        s.pop()
        assert len(s) == expected_len


@given(xs=small_lists)
def test_stack_prop_is_empty_iff_len_zero(xs: list[int]) -> None:
    """is_empty() ↔ len() == 0, всегда."""
    s: Stack[int] = Stack()
    for x in xs:
        s.push(x)
    assert s.is_empty() == (len(s) == 0)


# ─── Round-trip push / pop ─────────────────────────────────────────────────────

@given(xs=nonempty_lists)
def test_stack_prop_push_pop_roundtrip(xs: list[int]) -> None:
    """pop() возвращает то, что push() положил последним."""
    s: Stack[int] = Stack()
    for x in xs:
        s.push(x)
    assert s.pop() == xs[-1]


@given(xs=nonempty_lists)
def test_stack_prop_full_drain_lifo_order(xs: list[int]) -> None:
    """Полный слив pop() даёт элементы в обратном порядке push."""
    s: Stack[int] = Stack()
    for x in xs:
        s.push(x)
    result = []
    while not s.is_empty():
        result.append(s.pop())
    assert result == list(reversed(xs))


@given(xs=nonempty_lists)
def test_stack_prop_peek_equals_last_pushed(xs: list[int]) -> None:
    """peek() всегда равен последнему push, не удаляя его."""
    s: Stack[int] = Stack()
    for x in xs:
        s.push(x)
    assert s.peek() == xs[-1]


@given(xs=nonempty_lists)
def test_stack_prop_peek_stable(xs: list[int]) -> None:
    """Двойной peek() возвращает одно и то же значение."""
    s: Stack[int] = Stack()
    for x in xs:
        s.push(x)
    assert s.peek() == s.peek()


@given(xs=nonempty_lists)
def test_stack_prop_peek_does_not_change_len(xs: list[int]) -> None:
    """peek() не изменяет len()."""
    s: Stack[int] = Stack()
    for x in xs:
        s.push(x)
    before = len(s)
    s.peek()
    assert len(s) == before


# ─── Порядок итерации ─────────────────────────────────────────────────────────

@given(xs=small_lists)
def test_stack_prop_iter_top_to_bottom(xs: list[int]) -> None:
    """__iter__ идёт от вершины (последний push) к основанию (первый push)."""
    s: Stack[int] = Stack()
    for x in xs:
        s.push(x)
    assert list(s) == list(reversed(xs))


@given(xs=small_lists)
def test_stack_prop_iter_len_unchanged(xs: list[int]) -> None:
    """Итерация не меняет len()."""
    s: Stack[int] = Stack()
    for x in xs:
        s.push(x)
    before = len(s)
    _ = list(s)
    assert len(s) == before


@given(xs=small_lists)
def test_stack_prop_iter_idempotent(xs: list[int]) -> None:
    """Двойная итерация даёт одинаковый результат."""
    s: Stack[int] = Stack()
    for x in xs:
        s.push(x)
    assert list(s) == list(s)


# ─── Пустой стек ─────────────────────────────────────────────────────────────

@given(x=ints)
def test_stack_prop_pop_on_empty_raises(x: int) -> None:
    """pop() на пустом стеке бросает IndexError."""
    s: Stack[int] = Stack()
    with pytest.raises(IndexError):
        s.pop()


@given(x=ints)
def test_stack_prop_peek_on_empty_raises(x: int) -> None:
    """peek() на пустом стеке бросает IndexError."""
    s: Stack[int] = Stack()
    with pytest.raises(IndexError):
        s.peek()


@given(xs=nonempty_lists)
def test_stack_prop_drain_then_pop_raises(xs: list[int]) -> None:
    """pop() после полного слива бросает IndexError."""
    s: Stack[int] = Stack()
    for x in xs:
        s.push(x)
    while not s.is_empty():
        s.pop()
    with pytest.raises(IndexError):
        s.pop()


# ─── Инвариант: push затем pop возвращает то, что отправили ───────────────────

@given(before=small_lists, x=ints)
def test_stack_prop_push_peek_pop_invariant(before: list[int], x: int) -> None:
    """push(x) → peek() == x → pop() == x, len восстанавливается."""
    s: Stack[int] = Stack()
    for v in before:
        s.push(v)
    n_before = len(s)
    s.push(x)
    assert s.peek() == x
    assert s.pop() == x
    assert len(s) == n_before


# ─── Строковые элементы ───────────────────────────────────────────────────────

@given(xs=st.lists(strs, min_size=1, max_size=20))
def test_stack_prop_str_lifo(xs: list[str]) -> None:
    """Stack работает корректно для str-элементов (Generic)."""
    s: Stack[str] = Stack()
    for x in xs:
        s.push(x)
    result = []
    while not s.is_empty():
        result.append(s.pop())
    assert result == list(reversed(xs))


# ─── Независимость экземпляров ───────────────────────────────────────────────

@given(xs=small_lists, ys=small_lists)
def test_stack_prop_two_instances_independent(xs: list[int], ys: list[int]) -> None:
    """Два стека не разделяют состояние."""
    a: Stack[int] = Stack()
    b: Stack[int] = Stack()
    for x in xs:
        a.push(x)
    for y in ys:
        b.push(y)
    assert len(a) == len(xs)
    assert len(b) == len(ys)
