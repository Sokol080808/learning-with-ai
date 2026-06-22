// Тесты для задания 7: make_transformer / apply_all (Box<dyn Fn>).
// Эти тесты трогать не нужно — это эталон поведения. На стабе с todo!() они
// красные (паника внутри todo!()), на готовом решении — зелёные.
//
// Каждый файл в tests/ — отдельный интеграционный крейт; импортируем только
// публичный API.

use m11_iterators::*;

// ── make_transformer: детерминированные примеры ───────────────────────────────

#[test]
fn make_transformer_doubles() {
    let f = make_transformer(true);
    assert_eq!(f(5), 10);
}

#[test]
fn make_transformer_negates() {
    let f = make_transformer(false);
    assert_eq!(f(5), -5);
}

#[test]
fn make_transformer_doubles_edge_values() {
    let f = make_transformer(true);
    assert_eq!(f(0), 0);
    assert_eq!(f(-7), -14);
}

#[test]
fn make_transformer_negates_edge_values() {
    let f = make_transformer(false);
    assert_eq!(f(0), 0);
    assert_eq!(f(-3), 3);
}

#[test]
fn make_transformer_callable_many_times() {
    // Граница Fn (а не FnOnce) обещает многократный вызов одного и того же замыкания.
    let dbl = make_transformer(true);
    assert_eq!(dbl(2), 4);
    assert_eq!(dbl(2), 4);
    assert_eq!(dbl(100), 200);

    let neg = make_transformer(false);
    assert_eq!(neg(1), -1);
    assert_eq!(neg(1), -1);
    assert_eq!(neg(-50), 50);
}

#[test]
fn make_transformer_branches_are_independent() {
    // Две фабрики возвращают РАЗНЫЕ замыкания, не мешающие друг другу,
    // но обе имеют один и тот же стёртый тип Box<dyn Fn(i64) -> i64>.
    let dbl = make_transformer(true);
    let neg = make_transformer(false);
    assert_eq!(dbl(8), 16);
    assert_eq!(neg(8), -8);
}

// ── apply_all: детерминированные примеры ──────────────────────────────────────

#[test]
fn apply_all_spec_example() {
    let fns: Vec<Box<dyn Fn(i64) -> i64>> =
        vec![make_transformer(true), make_transformer(false)];
    assert_eq!(apply_all(&fns, 3), vec![6, -3]);
}

#[test]
fn apply_all_empty_slice() {
    let fns: Vec<Box<dyn Fn(i64) -> i64>> = vec![];
    let out: Vec<i64> = apply_all(&fns, 7);
    assert_eq!(out, vec![]);
}

#[test]
fn apply_all_preserves_order() {
    // Порядок результатов точно соответствует порядку замыканий в срезе.
    let fns: Vec<Box<dyn Fn(i64) -> i64>> = vec![
        make_transformer(false), // -x
        make_transformer(true),  // 2x
        make_transformer(false), // -x
    ];
    assert_eq!(apply_all(&fns, 10), vec![-10, 20, -10]);
}

#[test]
fn apply_all_single_closure() {
    let fns: Vec<Box<dyn Fn(i64) -> i64>> = vec![make_transformer(true)];
    assert_eq!(apply_all(&fns, -4), vec![-8]);
}

#[test]
fn apply_all_accepts_arbitrary_boxed_closures() {
    // apply_all работает с ЛЮБЫМИ Box<dyn Fn(i64) -> i64>, не только из make_transformer:
    // разнородные замыкания лежат в одном Vec благодаря стиранию типа.
    let add10: Box<dyn Fn(i64) -> i64> = Box::new(|x| x + 10);
    let square: Box<dyn Fn(i64) -> i64> = Box::new(|x| x * x);
    let fns = vec![add10, square];
    assert_eq!(apply_all(&fns, 4), vec![14, 16]);
}

// ── property-тесты ────────────────────────────────────────────────────────────
// Маленький детерминированный ГПСЧ (xorshift64*) — без внешних крейтов,
// воспроизводимо. Сид фиксированный (hardcode), диапазон скромный, чтобы оракул
// (2*x / -x) не переполнял i64.

fn next_u64(state: &mut u64) -> u64 {
    let mut x = *state;
    x ^= x >> 12;
    x ^= x << 25;
    x ^= x >> 27;
    *state = x;
    x.wrapping_mul(0x2545F4914F6CDD1D)
}

fn in_range(state: &mut u64, lo: i64, hi: i64) -> i64 {
    let span = (hi - lo + 1) as u64;
    lo + (next_u64(state) % span) as i64
}

/// make_transformer(true)(x) == 2*x для любого x (умеренный диапазон против overflow).
#[test]
fn prop_make_transformer_double_oracle() {
    let mut rng: u64 = 0x0BAD_C0DE_F00D_2025;
    for _ in 0..1000 {
        let x = in_range(&mut rng, -1_000_000, 1_000_000);
        assert_eq!(make_transformer(true)(x), 2 * x);
    }
}

/// make_transformer(false)(x) == -x для любого x.
#[test]
fn prop_make_transformer_negate_oracle() {
    let mut rng: u64 = 0xFACE_FEED_1357_9BDF;
    for _ in 0..1000 {
        let x = in_range(&mut rng, -1_000_000_000, 1_000_000_000);
        assert_eq!(make_transformer(false)(x), -x);
    }
}

/// Двойное отрицание возвращает исходное значение: neg(neg(x)) == x.
#[test]
fn prop_make_transformer_negate_involution() {
    let mut rng: u64 = 0x2468_ACE0_1122_3344;
    let neg = make_transformer(false);
    for _ in 0..1000 {
        let x = in_range(&mut rng, -1_000_000_000, 1_000_000_000);
        assert_eq!(neg(neg(x)), x);
    }
}

/// apply_all согласован с поэлементным вызовом замыканий (оракул — наивный цикл),
/// и длина результата равна числу замыканий.
#[test]
fn prop_apply_all_matches_pointwise() {
    let mut rng: u64 = 0xDEAD_BEEF_C0FF_EE00;
    for _ in 0..500 {
        let len = (next_u64(&mut rng) % 12) as usize;
        // Случайный набор удвоителей/отрицателей.
        let flags: Vec<bool> = (0..len).map(|_| next_u64(&mut rng) & 1 == 0).collect();
        let fns: Vec<Box<dyn Fn(i64) -> i64>> =
            flags.iter().map(|&b| make_transformer(b)).collect();

        let x = in_range(&mut rng, -1_000_000, 1_000_000);
        let got = apply_all(&fns, x);

        // оракул: применить каждое замыкание вручную, в том же порядке
        let expected: Vec<i64> = flags
            .iter()
            .map(|&b| if b { 2 * x } else { -x })
            .collect();

        assert_eq!(got.len(), len);
        assert_eq!(got, expected);
    }
}
