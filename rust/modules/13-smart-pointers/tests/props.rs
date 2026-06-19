// Рандомизированные property-тесты для модуля 13 — Умные указатели.
// Используют детерминированный ГПСЧ без внешних крейтов — прогон всегда воспроизводим.

use m13_smart_pointers::*;

// ---------------------------------------------------------------------------
// Детерминированный ГПСЧ (xorshift64*) — без внешних крейтов.
// Сид ФИКСИРОВАННЫЙ, прогон детерминирован.
// ---------------------------------------------------------------------------

/// маленький детерминированный ГПСЧ (xorshift64*), без внешних крейтов — воспроизводимо.
/// Используй ЭТОТ helper во всех property-тестах модуля. Сид ФИКСИРОВАННЫЙ (hardcode), чтобы прогон был детерминированным.
fn next_u64(state: &mut u64) -> u64 {
    let mut x = *state;
    x ^= x >> 12;
    x ^= x << 25;
    x ^= x >> 27;
    *state = x;
    x.wrapping_mul(0x2545F4914F6CDD1D)
}

/// целое в диапазоне [lo, hi] включительно (держи диапазон скромным, чтобы оракул не переполнял i64)
fn in_range(state: &mut u64, lo: i64, hi: i64) -> i64 {
    let span = (hi - lo + 1) as u64;
    lo + (next_u64(state) % span) as i64
}

// ---------------------------------------------------------------------------
// Вспомогательная функция: собирает Vec<i64> заданной длины из случайных значений.
// ---------------------------------------------------------------------------
fn random_vec(state: &mut u64, len: usize, lo: i64, hi: i64) -> Vec<i64> {
    (0..len).map(|_| in_range(state, lo, hi)).collect()
}

// ===========================================================================
// Тесты для List::len
// ===========================================================================

/// Свойство: длина Nil всегда 0.
#[test]
fn prop_nil_len_is_zero() {
    assert_eq!(List::Nil.len(), 0);
}

/// Свойство: len(build_list(v)) == v.len() для произвольного среза.
#[test]
fn prop_build_list_len_equals_input_count() {
    let mut rng: u64 = 0xDEAD_BEEF_1234_5678;
    for _ in 0..800 {
        let n = in_range(&mut rng, 0, 30) as usize;
        let v = random_vec(&mut rng, n, -10_000, 10_000);
        let list = build_list(&v);
        assert_eq!(
            list.len(),
            v.len(),
            "len mismatch for input {:?}",
            v
        );
    }
}

/// Свойство: len всегда >= 0 (проверяем через тип — usize не отрицательный,
/// но дополнительно убеждаемся, что непустой список имеет len >= 1).
#[test]
fn prop_nonempty_list_len_at_least_one() {
    let mut rng: u64 = 0xCAFE_BABE_DEAD_BEEF;
    for _ in 0..800 {
        let n = in_range(&mut rng, 1, 30) as usize; // минимум 1 элемент
        let v = random_vec(&mut rng, n, -10_000, 10_000);
        let list = build_list(&v);
        assert!(list.len() >= 1, "непустой список должен иметь len >= 1");
    }
}

// ===========================================================================
// Тесты для List::sum
// ===========================================================================

/// Оракул: sum(build_list(v)) == v.iter().sum::<i64>() для произвольного среза.
#[test]
fn prop_build_list_sum_equals_iter_sum() {
    let mut rng: u64 = 0x0102_0304_0506_0708;
    for _ in 0..800 {
        let n = in_range(&mut rng, 0, 30) as usize;
        let v = random_vec(&mut rng, n, -1_000, 1_000); // скромный диапазон — сумма не переполнит i64
        let list = build_list(&v);
        let expected: i64 = v.iter().sum();
        assert_eq!(
            list.sum(),
            expected,
            "sum mismatch for input {:?}",
            v
        );
    }
}

/// Свойство: sum пустого списка == 0.
#[test]
fn prop_nil_sum_is_zero() {
    assert_eq!(List::Nil.sum(), 0);
}

/// Свойство: если все элементы >= 0, то sum >= 0.
#[test]
fn prop_nonneg_elements_nonneg_sum() {
    let mut rng: u64 = 0xAAAA_BBBB_CCCC_DDDD;
    for _ in 0..800 {
        let n = in_range(&mut rng, 0, 30) as usize;
        let v = random_vec(&mut rng, n, 0, 1_000);
        let list = build_list(&v);
        assert!(
            list.sum() >= 0,
            "сумма неотрицательных элементов должна быть >= 0, got {} for {:?}",
            list.sum(),
            v
        );
    }
}

/// Свойство: если все элементы <= 0, то sum <= 0.
#[test]
fn prop_nonpos_elements_nonpos_sum() {
    let mut rng: u64 = 0x1111_2222_3333_4444;
    for _ in 0..800 {
        let n = in_range(&mut rng, 0, 30) as usize;
        let v = random_vec(&mut rng, n, -1_000, 0);
        let list = build_list(&v);
        assert!(
            list.sum() <= 0,
            "сумма неположительных элементов должна быть <= 0, got {} for {:?}",
            list.sum(),
            v
        );
    }
}

// ===========================================================================
// Тесты для build_list — round-trip и структурные свойства
// ===========================================================================

/// Round-trip: build_list(&[x]) даёт Cons(x, Box::new(Nil)).
#[test]
fn prop_build_list_single_element_structure() {
    let mut rng: u64 = 0xFEED_FACE_CAFE_BABE;
    for _ in 0..800 {
        let x = in_range(&mut rng, -10_000, 10_000);
        let list = build_list(&[x]);
        let expected = List::Cons(x, Box::new(List::Nil));
        assert_eq!(list, expected, "build_list(&[{x}]) должен вернуть Cons({x}, Nil)");
    }
}

/// Свойство: build_list(&[]) всегда равен List::Nil.
#[test]
fn prop_build_list_empty_is_nil() {
    // Детерминированная проверка, но как явный property-тест.
    for _ in 0..10 {
        assert_eq!(build_list(&[]), List::Nil);
    }
}

/// Свойство: len и sum согласованы — список из n одинаковых значений v
/// имеет sum == n as i64 * v.
#[test]
fn prop_uniform_list_sum_equals_n_times_value() {
    let mut rng: u64 = 0x9876_5432_10FE_DCBA;
    for _ in 0..800 {
        let n = in_range(&mut rng, 0, 20) as usize;
        let v = in_range(&mut rng, -100, 100);
        let items: Vec<i64> = vec![v; n];
        let list = build_list(&items);
        let expected_sum = n as i64 * v;
        assert_eq!(
            list.sum(),
            expected_sum,
            "uniform list of {n} × {v}: sum should be {expected_sum}"
        );
        assert_eq!(list.len(), n);
    }
}

/// Свойство: добавление элемента увеличивает len ровно на 1 и sum — на значение элемента.
#[test]
fn prop_prepend_increases_len_and_sum() {
    let mut rng: u64 = 0x5555_6666_7777_8888;
    for _ in 0..800 {
        let n = in_range(&mut rng, 0, 20) as usize;
        let v = random_vec(&mut rng, n, -500, 500);
        let extra = in_range(&mut rng, -500, 500);

        let list_orig = build_list(&v);
        let len_orig = list_orig.len();
        let sum_orig = list_orig.sum();

        // Добавляем extra в начало
        let mut extended = v.clone();
        extended.insert(0, extra);
        let list_ext = build_list(&extended);

        assert_eq!(list_ext.len(), len_orig + 1);
        assert_eq!(list_ext.sum(), sum_orig + extra);
    }
}

// ===========================================================================
// Тесты для Counter
// ===========================================================================

/// Свойство: n вызовов increment → get() == n.
#[test]
fn prop_counter_get_equals_increment_count() {
    let mut rng: u64 = 0xBEEF_DEAD_0001_0002;
    for _ in 0..500 {
        let n = in_range(&mut rng, 0, 1_000) as usize;
        let c = Counter::new();
        for _ in 0..n {
            c.increment();
        }
        assert_eq!(
            c.get(),
            n as i64,
            "после {n} вызовов increment get() должен вернуть {n}"
        );
    }
}

/// Свойство: каждый вызов increment увеличивает get() ровно на 1.
#[test]
fn prop_counter_monotone_increment() {
    let mut rng: u64 = 0xC001_C0DE_FACE_0001;
    for _ in 0..500 {
        let steps = in_range(&mut rng, 1, 200) as usize;
        let c = Counter::new();
        for i in 0..steps {
            let before = c.get();
            c.increment();
            let after = c.get();
            assert_eq!(
                after,
                before + 1,
                "шаг {i}: ожидали before+1={}, получили {}",
                before + 1,
                after
            );
        }
    }
}

/// Свойство: Counter::new() всегда начинается с 0 (многократно).
#[test]
fn prop_counter_new_always_zero() {
    for _ in 0..500 {
        let c = Counter::new();
        assert_eq!(c.get(), 0, "Counter::new() должен начинаться с 0");
    }
}

/// Свойство: Counter::default() совпадает с Counter::new() по начальному значению.
#[test]
fn prop_counter_default_same_as_new() {
    for _ in 0..200 {
        let d = Counter::default();
        let n = Counter::new();
        assert_eq!(d.get(), n.get(), "default() и new() должны давать одинаковое начальное значение");
    }
}

/// Свойство: мутация через &Counter (общая ссылка) работает корректно.
#[test]
fn prop_counter_mutates_through_shared_ref() {
    let mut rng: u64 = 0x1234_5678_ABCD_EF00;
    for _ in 0..500 {
        let n = in_range(&mut rng, 0, 500) as usize;
        let c = Counter::new();
        let shared: &Counter = &c;
        for _ in 0..n {
            shared.increment();
        }
        assert_eq!(
            shared.get(),
            n as i64,
            "increment через &Counter должен изменить состояние"
        );
    }
}

// ===========================================================================
// Тесты для rc_clone_count
// ===========================================================================

/// Свойство: rc_clone_count() всегда >= 2 (детерминированная, но явная property-проверка).
#[test]
fn prop_rc_clone_count_always_at_least_two() {
    // Функция детерминирована, но проверяем многократно — инвариант должен
    // оставаться в силе при каждом вызове.
    for _ in 0..200 {
        let count = rc_clone_count();
        assert!(
            count >= 2,
            "rc_clone_count() должен возвращать >= 2, получили {count}"
        );
    }
}
