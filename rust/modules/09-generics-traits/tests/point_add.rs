// Тесты для задания «перегрузка `+` через std::ops::Add для Point».
//
// Эти тесты трогать не нужно — это эталон поведения. Запускай:
//   ./rust/run.sh 09
// На стабе (`todo!()`) они красные; на твоей реализации `impl Add for Point` — зелёные.
//
// Каждый файл в tests/ — отдельный интеграционный крейт; импортируем только ПУБЛИЧНЫЙ API.

use m09_generics::*;

// ---------------------------------------------------------------------------
// Детерминированный ГПСЧ (xorshift64*) — без внешних крейтов, сид ФИКСИРОВАННЫЙ.
// Координаты держим ЦЕЛОЧИСЛЕННЫМИ (приводим к f64) — тогда сложение f64 точное и
// сравнения через `==` корректны (без эпсилона).
// ---------------------------------------------------------------------------

fn next_u64(state: &mut u64) -> u64 {
    let mut x = *state;
    x ^= x >> 12;
    x ^= x << 25;
    x ^= x >> 27;
    *state = x;
    x.wrapping_mul(0x2545F4914F6CDD1D)
}

/// целое в диапазоне [lo, hi] включительно (диапазон скромный — f64 точно представляет
/// все целые такого размера, а сумма не теряет точность).
fn in_range(state: &mut u64, lo: i64, hi: i64) -> i64 {
    let span = (hi - lo + 1) as u64;
    lo + (next_u64(state) % span) as i64
}

// ---------------------------------------------------------------------------
// Детерминированные тесты — контракт из спецификации.
// ---------------------------------------------------------------------------

#[test]
fn point_add_basic() {
    let p = Point { x: 1.0, y: 2.0 } + Point { x: 3.0, y: 4.0 };
    assert_eq!(p, Point { x: 4.0, y: 6.0 });
}

#[test]
fn point_add_with_zero() {
    let p = Point { x: 7.0, y: -3.0 } + Point { x: 0.0, y: 0.0 };
    assert_eq!(p, Point { x: 7.0, y: -3.0 });
}

#[test]
fn point_add_negative_coords() {
    let p = Point { x: -5.0, y: 10.0 } + Point { x: 2.0, y: -4.0 };
    assert_eq!(p, Point { x: -3.0, y: 6.0 });
}

/// Сложение точек коммутативно: a + b == b + a (на целочисленных координатах — точно).
#[test]
fn point_add_commutative_example() {
    let a = Point { x: 3.0, y: 8.0 };
    let b = Point { x: -1.0, y: 5.0 };
    assert_eq!(a + b, b + a);
}

/// Свидетельство ассоциированного типа: `<Point as Add>::Output == Point`.
///
/// Эта функция компилируется ТОЛЬКО если `type Output = Point` в твоём `impl`.
/// Она же — обобщённый код, который принимает любой тип, складывающийся «сам в себя»,
/// и здесь проверяет, что наш `Point` под него подходит, а результат — снова `Point`.
fn add_twice<T>(x: T) -> T
where
    T: std::ops::Add<Output = T> + Copy,
{
    x + x
}

#[test]
fn point_output_associated_type_is_point() {
    // Если бы Output был НЕ Point, эта строка не скомпилировалась бы:
    let doubled: Point = add_twice(Point { x: 2.0, y: 3.0 });
    assert_eq!(doubled, Point { x: 4.0, y: 6.0 });
}

// ---------------------------------------------------------------------------
// Property-тесты (seeded) — покоординатность, коммутативность, нейтральный 0.
// ---------------------------------------------------------------------------

/// Свойство: сложение покоординатно — `(a+b).x == a.x+b.x` и `(a+b).y == a.y+b.y`.
#[test]
fn prop_point_add_is_componentwise() {
    let mut rng: u64 = 0x0907_0907_1234_5678;
    for _ in 0..800 {
        let ax = in_range(&mut rng, -10_000, 10_000) as f64;
        let ay = in_range(&mut rng, -10_000, 10_000) as f64;
        let bx = in_range(&mut rng, -10_000, 10_000) as f64;
        let by = in_range(&mut rng, -10_000, 10_000) as f64;

        let a = Point { x: ax, y: ay };
        let b = Point { x: bx, y: by };
        let sum = a + b;

        assert_eq!(sum.x, ax + bx, "x-компонента: a={a:?}, b={b:?}");
        assert_eq!(sum.y, ay + by, "y-компонента: a={a:?}, b={b:?}");
    }
}

/// Свойство: коммутативность — `a + b == b + a` для произвольных точек.
#[test]
fn prop_point_add_commutative() {
    let mut rng: u64 = 0xCAFE_0009_BEEF_2025;
    for _ in 0..800 {
        let a = Point {
            x: in_range(&mut rng, -10_000, 10_000) as f64,
            y: in_range(&mut rng, -10_000, 10_000) as f64,
        };
        let b = Point {
            x: in_range(&mut rng, -10_000, 10_000) as f64,
            y: in_range(&mut rng, -10_000, 10_000) as f64,
        };
        assert_eq!(a + b, b + a, "коммутативность нарушена: a={a:?}, b={b:?}");
    }
}

/// Свойство: нулевая точка — нейтральный элемент (`p + origin == p`).
#[test]
fn prop_point_add_zero_is_identity() {
    let mut rng: u64 = 0x1357_9BDF_0246_8ACE;
    let origin = Point { x: 0.0, y: 0.0 };
    for _ in 0..800 {
        let p = Point {
            x: in_range(&mut rng, -10_000, 10_000) as f64,
            y: in_range(&mut rng, -10_000, 10_000) as f64,
        };
        assert_eq!(p + origin, p, "origin должен быть нейтральным: p={p:?}");
        assert_eq!(origin + p, p, "origin должен быть нейтральным слева: p={p:?}");
    }
}
