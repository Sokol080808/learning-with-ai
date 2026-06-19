// Тесты для задания 5: вычислитель выражений (Expr + eval).
// Два класса тестов:
//   (a) детерминированные примеры — конкретные входные данные → конкретный результат;
//   (b) рандомизированные property-тесты — инварианты и оракул, детерминированный xorshift64*.
//
// Все вызовы eval / Expr происходят в рантайме внутри #[test] функций,
// поэтому тесты компилируются против стаба с todo!() (только упадут в рантайме).

use m06_enums::{eval, Expr};

// ── вспомогательные функции ──────────────────────────────────────────────────

/// Допуск для сравнения f64.
fn approx(a: f64, b: f64) -> bool {
    (a - b).abs() < 1e-9
}

/// Детерминированный ГПСЧ xorshift64* — тот же алгоритм, что в props.rs.
fn next_u64(state: &mut u64) -> u64 {
    let mut x = *state;
    x ^= x >> 12;
    x ^= x << 25;
    x ^= x >> 27;
    *state = x;
    x.wrapping_mul(0x2545F4914F6CDD1D)
}

/// Целое в диапазоне [lo, hi] включительно.
fn in_range(state: &mut u64, lo: i64, hi: i64) -> i64 {
    let span = (hi - lo + 1) as u64;
    lo + (next_u64(state) % span) as i64
}

/// Случайное f64 в диапазоне [lo, hi] с шагом 0.01.
fn rand_f64(state: &mut u64, lo: i64, hi: i64) -> f64 {
    in_range(state, lo * 100, hi * 100) as f64 / 100.0
}

// Удобные конструкторы — сокращают шум в тестах.
fn num(n: f64) -> Expr {
    Expr::Num(n)
}
fn add(l: Expr, r: Expr) -> Expr {
    Expr::Add(Box::new(l), Box::new(r))
}
fn sub(l: Expr, r: Expr) -> Expr {
    Expr::Sub(Box::new(l), Box::new(r))
}
fn mul(l: Expr, r: Expr) -> Expr {
    Expr::Mul(Box::new(l), Box::new(r))
}
fn div(l: Expr, r: Expr) -> Expr {
    Expr::Div(Box::new(l), Box::new(r))
}

// ── (a) Детерминированные примеры ────────────────────────────────────────────

// Листовой узел.
#[test]
fn leaf_num_returns_its_value() {
    assert_eq!(eval(&num(0.0)), Some(0.0));
    assert_eq!(eval(&num(42.0)), Some(42.0));
    assert_eq!(eval(&num(-7.5)), Some(-7.5));
}

// Простые арифметические операции над двумя числами.
#[test]
fn add_two_numbers() {
    let e = add(num(3.0), num(4.0));
    assert!(approx(eval(&e).unwrap(), 7.0));
}

#[test]
fn sub_two_numbers() {
    let e = sub(num(10.0), num(3.0));
    assert!(approx(eval(&e).unwrap(), 7.0));
}

#[test]
fn mul_two_numbers() {
    let e = mul(num(3.0), num(4.0));
    assert!(approx(eval(&e).unwrap(), 12.0));
}

#[test]
fn div_two_numbers() {
    let e = div(num(10.0), num(4.0));
    assert!(approx(eval(&e).unwrap(), 2.5));
}

// Деление на ноль → None.
#[test]
fn div_by_zero_leaf_is_none() {
    let e = div(num(5.0), num(0.0));
    assert_eq!(eval(&e), None);
}

// Ноль делим на что-то → Some(0).
#[test]
fn zero_divided_by_nonzero_is_zero() {
    let e = div(num(0.0), num(7.0));
    assert!(approx(eval(&e).unwrap(), 0.0));
}

// None распространяется через Add наверх.
#[test]
fn div_by_zero_propagates_through_add() {
    // (1 / 0) + 2 — левое поддерево — None, итог должен быть None.
    let e = add(div(num(1.0), num(0.0)), num(2.0));
    assert_eq!(eval(&e), None);
}

// None распространяется через Mul наверх.
#[test]
fn div_by_zero_propagates_through_mul() {
    let e = mul(num(3.0), div(num(5.0), num(0.0)));
    assert_eq!(eval(&e), None);
}

// None распространяется через Sub.
#[test]
fn div_by_zero_propagates_through_sub() {
    let e = sub(div(num(8.0), num(0.0)), num(1.0));
    assert_eq!(eval(&e), None);
}

// None распространяется через Div знаменатель.
#[test]
fn div_by_zero_propagates_through_div_denominator() {
    // 10 / (5 / 0) → None
    let e = div(num(10.0), div(num(5.0), num(0.0)));
    assert_eq!(eval(&e), None);
}

// Вложенное дерево без нулей считается корректно.
#[test]
fn nested_arithmetic_no_div_by_zero() {
    // (2 + 3) * (10 - 4) = 5 * 6 = 30
    let e = mul(add(num(2.0), num(3.0)), sub(num(10.0), num(4.0)));
    assert!(approx(eval(&e).unwrap(), 30.0));
}

// Глубокое дерево: ((1+2)*(3+4)) / ((2*3)-(1*1)) = 21 / 5 = 4.2
#[test]
fn deep_tree_evaluates_correctly() {
    let lhs = mul(add(num(1.0), num(2.0)), add(num(3.0), num(4.0)));
    let rhs = sub(mul(num(2.0), num(3.0)), mul(num(1.0), num(1.0)));
    let e = div(lhs, rhs);
    assert!(approx(eval(&e).unwrap(), 4.2));
}

// Деление на ноль глубоко в дереве всплывает через несколько уровней.
#[test]
fn div_by_zero_deep_propagates() {
    // ((5 / 0) + 3) * 4 → None
    let e = mul(add(div(num(5.0), num(0.0)), num(3.0)), num(4.0));
    assert_eq!(eval(&e), None);
}

// Отрицательные числа.
#[test]
fn negative_numbers_work() {
    let e = add(num(-3.0), num(-4.0));
    assert!(approx(eval(&e).unwrap(), -7.0));
}

// Деление с отрицательным делителем.
#[test]
fn div_negative_denominator() {
    let e = div(num(6.0), num(-2.0));
    assert!(approx(eval(&e).unwrap(), -3.0));
}

// Нулевое число (не деление) не даёт None.
#[test]
fn zero_num_is_some_zero() {
    let e = add(num(0.0), num(0.0));
    assert_eq!(eval(&e), Some(0.0));
}

// ── (b) Рандомизированные property-тесты ────────────────────────────────────

/// Инвариант: eval(Num(x)) == Some(x) для любого x.
#[test]
fn prop_eval_num_always_some() {
    let mut rng: u64 = 0xABCD_1234_EF56_7890;
    for _ in 0..1000 {
        let x = rand_f64(&mut rng, -1000, 1000);
        let e = num(x);
        assert_eq!(eval(&e), Some(x));
    }
}

/// Инвариант: eval(Add(Num(a), Num(b))) == Some(a + b).
#[test]
fn prop_eval_add_leaves_oracle() {
    let mut rng: u64 = 0x1111_AAAA_2222_BBBB;
    for _ in 0..1000 {
        let a = rand_f64(&mut rng, -500, 500);
        let b = rand_f64(&mut rng, -500, 500);
        let e = add(num(a), num(b));
        let got = eval(&e).expect("Add двух чисел не должно давать None");
        assert!(
            approx(got, a + b),
            "Add({a},{b}): eval={got}, ожидалось {}",
            a + b
        );
    }
}

/// Инвариант: eval(Sub(Num(a), Num(b))) == Some(a - b).
#[test]
fn prop_eval_sub_leaves_oracle() {
    let mut rng: u64 = 0x3333_CCCC_4444_DDDD;
    for _ in 0..1000 {
        let a = rand_f64(&mut rng, -500, 500);
        let b = rand_f64(&mut rng, -500, 500);
        let e = sub(num(a), num(b));
        let got = eval(&e).expect("Sub двух чисел не должно давать None");
        assert!(approx(got, a - b), "Sub({a},{b}): eval={got}, ожидалось {}", a - b);
    }
}

/// Инвариант: eval(Mul(Num(a), Num(b))) == Some(a * b).
#[test]
fn prop_eval_mul_leaves_oracle() {
    let mut rng: u64 = 0x5555_EEEE_6666_FFFF;
    for _ in 0..1000 {
        let a = rand_f64(&mut rng, -100, 100);
        let b = rand_f64(&mut rng, -100, 100);
        let e = mul(num(a), num(b));
        let got = eval(&e).expect("Mul двух чисел не должно давать None");
        assert!(approx(got, a * b), "Mul({a},{b}): eval={got}, ожидалось {}", a * b);
    }
}

/// Инвариант: Div(Num(a), Num(b)) с b != 0 всегда Some(a/b).
#[test]
fn prop_eval_div_nonzero_oracle() {
    let mut rng: u64 = 0x7777_0000_8888_1111;
    for _ in 0..1000 {
        let a = rand_f64(&mut rng, -500, 500);
        // b из [1, 1000]/100 — строго положительный, ненулевой
        let b = in_range(&mut rng, 1, 1000) as f64 / 100.0;
        let e = div(num(a), num(b));
        let got = eval(&e).expect("Div(a, b!=0) не должно давать None");
        assert!(
            approx(got, a / b),
            "Div({a},{b}): eval={got}, ожидалось {}",
            a / b
        );
    }
}

/// Инвариант: Div(Num(a), Num(0.0)) всегда None, для любого a.
#[test]
fn prop_eval_div_zero_always_none() {
    let mut rng: u64 = 0x9999_2222_AAAA_3333;
    for _ in 0..1000 {
        let a = rand_f64(&mut rng, -500, 500);
        let e = div(num(a), num(0.0));
        assert_eq!(eval(&e), None, "Div({a}, 0.0) должно быть None");
    }
}

/// Инвариант коммутативности: Add(a,b) == Add(b,a).
#[test]
fn prop_add_commutative() {
    let mut rng: u64 = 0xBBBB_4444_CCCC_5555;
    for _ in 0..500 {
        let a = rand_f64(&mut rng, -500, 500);
        let b = rand_f64(&mut rng, -500, 500);
        let ab = eval(&add(num(a), num(b))).unwrap();
        let ba = eval(&add(num(b), num(a))).unwrap();
        assert!(approx(ab, ba), "Add не коммутативен: {ab} != {ba}");
    }
}

/// Инвариант: умножение на 1 не меняет значение.
#[test]
fn prop_mul_identity_one() {
    let mut rng: u64 = 0xDDDD_6666_EEEE_7777;
    for _ in 0..500 {
        let a = rand_f64(&mut rng, -500, 500);
        let e = mul(num(a), num(1.0));
        let got = eval(&e).unwrap();
        assert!(approx(got, a), "Mul({a}, 1.0) = {got}, ожидалось {a}");
    }
}

/// Инвариант: умножение на 0 даёт 0.
#[test]
fn prop_mul_zero_annihilates() {
    let mut rng: u64 = 0xFFFF_8888_0000_9999;
    for _ in 0..500 {
        let a = rand_f64(&mut rng, -500, 500);
        let e = mul(num(a), num(0.0));
        let got = eval(&e).unwrap();
        assert!(approx(got, 0.0), "Mul({a}, 0.0) = {got}, ожидалось 0.0");
    }
}

/// Инвариант: a/a == 1 при a != 0.
#[test]
fn prop_div_self_is_one() {
    let mut rng: u64 = 0x1234_5678_9ABC_DEF0;
    for _ in 0..500 {
        // выбираем a != 0
        let a = (in_range(&mut rng, 1, 1000) as f64) / 100.0;
        let sign = if next_u64(&mut rng) & 1 == 0 { 1.0 } else { -1.0 };
        let a = a * sign;
        let e = div(num(a), num(a));
        let got = eval(&e).expect("Div(a,a) для a!=0 должен быть Some");
        assert!(approx(got, 1.0), "Div({a},{a}) = {got}, ожидалось 1.0");
    }
}

/// Оракул против встроенной арифметики: Add(Sub(a,b), Mul(c,d)) == (a-b) + (c*d).
#[test]
fn prop_composite_tree_matches_inline_oracle() {
    let mut rng: u64 = 0xFEDC_BA98_7654_3210;
    for _ in 0..500 {
        let a = rand_f64(&mut rng, -100, 100);
        let b = rand_f64(&mut rng, -100, 100);
        let c = rand_f64(&mut rng, -100, 100);
        let d = rand_f64(&mut rng, -100, 100);
        let e = add(sub(num(a), num(b)), mul(num(c), num(d)));
        let got = eval(&e).expect("составное дерево без деления не должно давать None");
        let expected = (a - b) + (c * d);
        assert!(
            approx(got, expected),
            "составное дерево: eval={got}, оракул={expected}"
        );
    }
}

/// None не может прийти из Add/Sub/Mul без участия Div.
#[test]
fn prop_no_div_means_always_some() {
    let mut rng: u64 = 0xA1B2_C3D4_E5F6_0718;
    for _ in 0..500 {
        let a = rand_f64(&mut rng, -100, 100);
        let b = rand_f64(&mut rng, -100, 100);
        let c = rand_f64(&mut rng, -100, 100);
        // Дерево Add(Mul(a,b), Sub(c, a)) — никакого Div
        let e = add(mul(num(a), num(b)), sub(num(c), num(a)));
        assert!(
            eval(&e).is_some(),
            "Add/Mul/Sub дерево вернуло None для a={a} b={b} c={c}"
        );
    }
}
