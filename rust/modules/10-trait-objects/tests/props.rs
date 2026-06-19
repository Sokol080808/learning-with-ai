// Randomized property tests for m10_traits (module 10 — trait objects).
//
// All tests use a deterministic xorshift64* PRNG with a fixed seed — fully
// reproducible, no external crates needed.

use m10_traits::*;
use std::f64::consts::PI;

// ─── детерминированный ГПСЧ (xorshift64*) ────────────────────────────────────
// маленький детерминированный ГПСЧ (xorshift64*), без внешних крейтов — воспроизводимо.
// Используй ЭТОТ helper во всех property-тестах модуля. Сид ФИКСИРОВАННЫЙ (hardcode), чтобы прогон был детерминированным.
fn next_u64(state: &mut u64) -> u64 {
    let mut x = *state;
    x ^= x >> 12;
    x ^= x << 25;
    x ^= x >> 27;
    *state = x;
    x.wrapping_mul(0x2545F4914F6CDD1D)
}
// целое в диапазоне [lo, hi] включительно (держи диапазон скромным, чтобы оракул не переполнял i64)
fn in_range(state: &mut u64, lo: i64, hi: i64) -> i64 {
    let span = (hi - lo + 1) as u64;
    lo + (next_u64(state) % span) as i64
}

// Positive f64 from a u64 state, scaled to [0.0, max].
fn pos_f64(state: &mut u64, max: f64) -> f64 {
    let bits = next_u64(state);
    // map to (0.0, 1.0] then scale
    (bits as f64 / u64::MAX as f64) * max
}

// ─── Задание 1. Свойства Circle::area ────────────────────────────────────────

/// area() для круга всегда неотрицательна.
#[test]
fn prop_circle_area_non_negative() {
    let mut rng: u64 = 0xDEAD_BEEF_1234_5678;
    for _ in 0..1000 {
        let r = pos_f64(&mut rng, 10_000.0);
        let c = Circle { r };
        assert!(c.area() >= 0.0, "circle area must be >= 0, got {} for r={}", c.area(), r);
    }
}

/// area() для круга должна совпадать с оракулом π·r².
#[test]
fn prop_circle_area_oracle() {
    let mut rng: u64 = 0xABCD_EF01_2345_6789;
    for _ in 0..1000 {
        let r = pos_f64(&mut rng, 10_000.0);
        let c = Circle { r };
        let expected = PI * r * r;
        let diff = (c.area() - expected).abs();
        assert!(diff < 1e-6,
            "circle area oracle mismatch: got {}, expected {}, diff {} for r={}",
            c.area(), expected, diff, r);
    }
}

/// Масштабирование: Circle { r: 2r }.area() == 4 * Circle { r }.area().
#[test]
fn prop_circle_area_scales_quadratically() {
    let mut rng: u64 = 0x1111_2222_3333_4444;
    for _ in 0..800 {
        let r = pos_f64(&mut rng, 1_000.0);
        let a1 = Circle { r }.area();
        let a2 = Circle { r: r * 2.0 }.area();
        let diff = (a2 - 4.0 * a1).abs();
        assert!(diff < 1e-4,
            "scaling failed: r={}, area(r)={}, area(2r)={}", r, a1, a2);
    }
}

// ─── Задание 1. Свойства Square::area ────────────────────────────────────────

/// area() для квадрата всегда неотрицательна.
#[test]
fn prop_square_area_non_negative() {
    let mut rng: u64 = 0xCAFE_BABE_0000_0001;
    for _ in 0..1000 {
        let side = pos_f64(&mut rng, 10_000.0);
        let s = Square { side };
        assert!(s.area() >= 0.0, "square area must be >= 0 for side={}", side);
    }
}

/// area() для квадрата совпадает с оракулом side².
#[test]
fn prop_square_area_oracle() {
    let mut rng: u64 = 0xFEED_FACE_DEAD_BEEF;
    for _ in 0..1000 {
        let side = pos_f64(&mut rng, 10_000.0);
        let s = Square { side };
        let expected = side * side;
        let diff = (s.area() - expected).abs();
        assert!(diff < 1e-6,
            "square area oracle mismatch: got {}, expected {}, diff {}", s.area(), expected, diff);
    }
}

/// Масштабирование: Square { side: 3s }.area() == 9 * Square { side: s }.area().
#[test]
fn prop_square_area_scales_quadratically() {
    let mut rng: u64 = 0x5555_6666_7777_8888;
    for _ in 0..800 {
        let side = pos_f64(&mut rng, 1_000.0);
        let a1 = Square { side }.area();
        let a3 = Square { side: side * 3.0 }.area();
        let diff = (a3 - 9.0 * a1).abs();
        assert!(diff < 1e-3,
            "scaling failed: side={}, area(s)={}, area(3s)={}", side, a1, a3);
    }
}

// ─── Задание 1. Свойства total_area ──────────────────────────────────────────

/// total_area совпадает с ручным суммированием через oracle-цикл.
#[test]
fn prop_total_area_equals_manual_sum() {
    let mut rng: u64 = 0x9999_AAAA_BBBB_CCCC;
    for _ in 0..500 {
        let n = (next_u64(&mut rng) % 20) as usize; // 0..19 shapes
        let mut shapes: Vec<Box<dyn Shape>> = Vec::new();
        let mut expected_sum = 0.0_f64;
        for _ in 0..n {
            let kind = next_u64(&mut rng) % 2;
            if kind == 0 {
                let r = pos_f64(&mut rng, 100.0);
                expected_sum += PI * r * r;
                shapes.push(Box::new(Circle { r }));
            } else {
                let side = pos_f64(&mut rng, 100.0);
                expected_sum += side * side;
                shapes.push(Box::new(Square { side }));
            }
        }
        let got = total_area(&shapes);
        let diff = (got - expected_sum).abs();
        assert!(diff < 1e-4,
            "total_area mismatch: got={}, expected={}, diff={}, n={}",
            got, expected_sum, diff, n);
    }
}

/// total_area всегда неотрицательна.
#[test]
fn prop_total_area_non_negative() {
    let mut rng: u64 = 0xDDDD_EEEE_FFFF_0000;
    for _ in 0..500 {
        let n = (next_u64(&mut rng) % 20) as usize;
        let mut shapes: Vec<Box<dyn Shape>> = Vec::new();
        for _ in 0..n {
            if next_u64(&mut rng) % 2 == 0 {
                shapes.push(Box::new(Circle { r: pos_f64(&mut rng, 100.0) }));
            } else {
                shapes.push(Box::new(Square { side: pos_f64(&mut rng, 100.0) }));
            }
        }
        assert!(total_area(&shapes) >= 0.0);
    }
}

/// total_area монотонна: добавление фигуры с r>0 только увеличивает сумму.
#[test]
fn prop_total_area_monotone_append() {
    let mut rng: u64 = 0x0102_0304_0506_0708;
    for _ in 0..500 {
        let n = (next_u64(&mut rng) % 10) as usize;
        let mut shapes: Vec<Box<dyn Shape>> = Vec::new();
        for _ in 0..n {
            shapes.push(Box::new(Square { side: pos_f64(&mut rng, 50.0) }));
        }
        let before = total_area(&shapes);
        let extra_r = pos_f64(&mut rng, 50.0) + 0.001; // strictly positive
        shapes.push(Box::new(Circle { r: extra_r }));
        let after = total_area(&shapes);
        assert!(after >= before,
            "total_area decreased after adding a shape: before={}, after={}", before, after);
    }
}

// ─── Задание 2. Свойства Point Display ───────────────────────────────────────

/// format!("{}", Point { x, y }) парсится обратно в исходные координаты (round-trip).
/// Используем только целочисленные значения, чтобы Display не зависел от десятичных
/// артефактов f64::Display.
#[test]
fn prop_point_display_round_trip_integers() {
    let mut rng: u64 = 0xBEEF_CAFE_1234_ABCD;
    for _ in 0..1000 {
        let xi = in_range(&mut rng, -1000, 1000);
        let yi = in_range(&mut rng, -1000, 1000);
        let x = xi as f64;
        let y = yi as f64;
        let p = Point { x, y };
        let s = format!("{}", p);
        // Expected format: "(xi, yi)"
        let expected = format!("({}, {})", xi, yi);
        assert_eq!(s, expected,
            "Point display mismatch: got '{}', expected '{}'", s, expected);
    }
}

/// to_string() и format!("{}") возвращают одно и то же.
#[test]
fn prop_point_to_string_equals_format() {
    let mut rng: u64 = 0x1A2B_3C4D_5E6F_7080;
    for _ in 0..1000 {
        let xi = in_range(&mut rng, -500, 500);
        let yi = in_range(&mut rng, -500, 500);
        let p = Point { x: xi as f64, y: yi as f64 };
        assert_eq!(p.to_string(), format!("{}", p),
            "to_string() != format!(\"{{}}\") for {:?}", p);
    }
}

/// Формат всегда начинается с '(' и заканчивается ')'.
#[test]
fn prop_point_display_parens() {
    let mut rng: u64 = 0x0011_2233_4455_6677;
    for _ in 0..1000 {
        let p = Point {
            x: in_range(&mut rng, -10_000, 10_000) as f64,
            y: in_range(&mut rng, -10_000, 10_000) as f64,
        };
        let s = format!("{}", p);
        assert!(s.starts_with('('), "display must start with '(': got '{}'", s);
        assert!(s.ends_with(')'),   "display must end with ')': got '{}'", s);
    }
}

/// Формат содержит разделитель ", " между координатами.
#[test]
fn prop_point_display_contains_separator() {
    let mut rng: u64 = 0x8899_AABB_CCDD_EEFF;
    for _ in 0..1000 {
        let p = Point {
            x: in_range(&mut rng, -10_000, 10_000) as f64,
            y: in_range(&mut rng, -10_000, 10_000) as f64,
        };
        let s = format!("{}", p);
        assert!(s.contains(", "),
            "display must contain ', ' separator: got '{}'", s);
    }
}

// ─── Задание 3. Свойства Counter / counter_sum ───────────────────────────────

/// Counter::new().collect() всегда даёт ровно 5 элементов.
#[test]
fn prop_counter_always_yields_five_elements() {
    // Counter is deterministic, but we verify the property repeatedly
    // by independently creating fresh counters (no random input needed here;
    // we use the loop to document it as a property, not an example).
    for _ in 0..1000 {
        let v: Vec<u32> = Counter::new().collect();
        assert_eq!(v.len(), 5, "Counter must yield exactly 5 elements");
    }
}

/// Элементы Counter строго возрастают на 1 (1, 2, 3, 4, 5).
#[test]
fn prop_counter_elements_strictly_increasing_by_one() {
    for _ in 0..1000 {
        let v: Vec<u32> = Counter::new().collect();
        for w in v.windows(2) {
            assert_eq!(w[1], w[0] + 1,
                "Counter elements must increase by 1 each step: {:?}", v);
        }
    }
}

/// Первый элемент Counter равен 1, последний равен 5.
#[test]
fn prop_counter_first_and_last() {
    for _ in 0..1000 {
        let v: Vec<u32> = Counter::new().collect();
        assert_eq!(*v.first().unwrap(), 1, "Counter first element must be 1");
        assert_eq!(*v.last().unwrap(), 5, "Counter last element must be 5");
    }
}

/// После 5 вызовов next() дальнейшие вызовы всегда возвращают None.
#[test]
fn prop_counter_none_after_exhaustion() {
    for _ in 0..1000 {
        let mut c = Counter::new();
        for _ in 0..5 { c.next(); }
        // Verify None repeats
        for extra in 0..10 {
            assert_eq!(c.next(), None,
                "Counter must return None after exhaustion (call #{})", extra + 1);
        }
    }
}

/// sum() совпадает с оракулом: арифметическая прогрессия 1..=5.
#[test]
fn prop_counter_sum_oracle() {
    // Oracle: sum of 1..=n = n*(n+1)/2; for n=5 → 15.
    let oracle: u32 = (1_u32..=5).sum();
    for _ in 0..1000 {
        assert_eq!(Counter::new().sum::<u32>(), oracle,
            "Counter sum must equal oracle {}", oracle);
    }
}

/// counter_sum() == Counter::new().sum() (функция-обёртка согласована с прямым вызовом).
#[test]
fn prop_counter_sum_fn_matches_direct() {
    for _ in 0..1000 {
        let direct: u32 = Counter::new().sum();
        assert_eq!(counter_sum(), direct,
            "counter_sum() must match Counter::new().sum()");
    }
}

/// Counter::new() всегда стартует с count==0.
#[test]
fn prop_counter_new_starts_at_zero() {
    for _ in 0..1000 {
        let c = Counter::new();
        assert_eq!(c.count, 0, "Counter::new() must start with count=0");
    }
}

/// Свойство корректности через collect + oracle slice.
#[test]
fn prop_counter_collect_equals_one_to_five() {
    let oracle = vec![1_u32, 2, 3, 4, 5];
    for _ in 0..1000 {
        let v: Vec<u32> = Counter::new().collect();
        assert_eq!(v, oracle, "Counter collect must equal [1,2,3,4,5]");
    }
}
