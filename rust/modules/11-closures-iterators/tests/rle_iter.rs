// Тесты для RunLengthEncoder — адаптера «кодирование длин серий».
// Трогать не нужно — это эталон поведения.
//
// Файл организован в два раздела:
//   1. Детерминированные примеры (конкретные входы → конкретные выходы).
//   2. Рандомизированные property-тесты (инварианты + оракул на naive-цикл).

use m11_iterators::RunLengthEncoder;

// ─────────────────────────────────────────────────────────────────────────────
// Вспомогательная функция: запускает RunLengthEncoder и собирает результат.
// ─────────────────────────────────────────────────────────────────────────────

fn rle(input: &[i64]) -> Vec<(i64, usize)> {
    RunLengthEncoder::new(input.iter().copied()).collect()
}

// ─────────────────────────────────────────────────────────────────────────────
// Раздел 1. Детерминированные тесты
// ─────────────────────────────────────────────────────────────────────────────

/// Пустой вход — итератор должен немедленно вернуть None.
#[test]
fn rle_empty_input() {
    assert_eq!(rle(&[]), vec![]);
}

/// Один элемент — единственная пара (значение, 1).
#[test]
fn rle_single_element() {
    assert_eq!(rle(&[42]), vec![(42, 1)]);
}

/// Два одинаковых элемента — одна пара с count=2.
#[test]
fn rle_two_same() {
    assert_eq!(rle(&[7, 7]), vec![(7, 2)]);
}

/// Два разных элемента — две пары по одному.
#[test]
fn rle_two_different() {
    assert_eq!(rle(&[3, 5]), vec![(3, 1), (5, 1)]);
}

/// Базовый пример из спецификации: [1,1,2,3,3,3] → [(1,2),(2,1),(3,3)].
#[test]
fn rle_spec_example() {
    assert_eq!(rle(&[1, 1, 2, 3, 3, 3]), vec![(1, 2), (2, 1), (3, 3)]);
}

/// Все элементы одинаковые — одна пара с полным count.
#[test]
fn rle_all_same() {
    assert_eq!(rle(&[5, 5, 5, 5, 5]), vec![(5, 5)]);
}

/// Все элементы разные — каждый образует пару (x, 1).
#[test]
fn rle_all_different() {
    assert_eq!(
        rle(&[10, 20, 30, 40]),
        vec![(10, 1), (20, 1), (30, 1), (40, 1)]
    );
}

/// Прогон в начале, одиночки в конце.
#[test]
fn rle_run_at_start() {
    assert_eq!(rle(&[9, 9, 9, 1, 2]), vec![(9, 3), (1, 1), (2, 1)]);
}

/// Одиночки в начале, прогон в конце.
#[test]
fn rle_run_at_end() {
    assert_eq!(rle(&[1, 2, 7, 7, 7, 7]), vec![(1, 1), (2, 1), (7, 4)]);
}

/// Отрицательные числа и ноль обрабатываются корректно.
#[test]
fn rle_negatives_and_zero() {
    assert_eq!(
        rle(&[-3, -3, 0, 0, 0, -1]),
        vec![(-3, 2), (0, 3), (-1, 1)]
    );
}

/// Прогоны чередуются без повторений группами.
#[test]
fn rle_alternating_groups() {
    assert_eq!(
        rle(&[1, 1, 2, 2, 1, 1]),
        vec![(1, 2), (2, 2), (1, 2)]
    );
}

/// Одиночный элемент с большим значением.
#[test]
fn rle_large_value() {
    assert_eq!(rle(&[i64::MAX]), vec![(i64::MAX, 1)]);
}

/// Прогон из одного элемента с min значением.
#[test]
fn rle_min_value() {
    assert_eq!(rle(&[i64::MIN, i64::MIN]), vec![(i64::MIN, 2)]);
}

/// Проверяем, что адаптер ленив и корректно обрабатывает произвольную длину прогона.
#[test]
fn rle_long_run() {
    let input: Vec<i64> = vec![42; 1000];
    assert_eq!(rle(&input), vec![(42, 1000)]);
}

/// После окончания итератора повторный вызов next возвращает None.
#[test]
fn rle_exhausted_returns_none() {
    let mut enc = RunLengthEncoder::new([1i64, 2].iter().copied());
    assert_eq!(enc.next(), Some((1, 1)));
    assert_eq!(enc.next(), Some((2, 1)));
    assert_eq!(enc.next(), None);
    // повторный вызов — тоже None
    assert_eq!(enc.next(), None);
}

/// Общее количество элементов в парах == длина входа.
#[test]
fn rle_total_count_equals_input_len() {
    let input = [1i64, 1, 2, 3, 3, 3, 4, 4];
    let pairs = rle(&input);
    let total: usize = pairs.iter().map(|(_, c)| c).sum();
    assert_eq!(total, input.len());
}

/// Декодирование (повтор каждого значения count раз) восстанавливает вход.
#[test]
fn rle_decode_roundtrip_concrete() {
    let input = [1i64, 1, 2, 3, 3, 3, 4];
    let encoded = rle(&input);
    let decoded: Vec<i64> = encoded
        .iter()
        .flat_map(|&(val, count)| std::iter::repeat(val).take(count))
        .collect();
    assert_eq!(decoded, input);
}

// ─────────────────────────────────────────────────────────────────────────────
// Раздел 2. Рандомизированные property-тесты
//
// ГПСЧ: xorshift64* с фиксированным сидом (без внешних крейтов).
// ─────────────────────────────────────────────────────────────────────────────

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

/// Генерирует случайный вектор i64 с «кластеризацией» (серии повторяющихся
/// значений встречаются естественно).
fn gen_input(rng: &mut u64, max_len: usize) -> Vec<i64> {
    let len = (next_u64(rng) % (max_len as u64 + 1)) as usize;
    (0..len).map(|_| in_range(rng, -20, 20)).collect()
}

/// Наивный оракул: считает серии через обычный цикл.
fn naive_rle(input: &[i64]) -> Vec<(i64, usize)> {
    let mut result = Vec::new();
    let mut iter = input.iter().peekable();
    while let Some(&val) = iter.next() {
        let mut count = 1usize;
        while iter.peek() == Some(&&val) {
            iter.next();
            count += 1;
        }
        result.push((val, count));
    }
    result
}

// ── Инвариант 1: результат нашего адаптера == оракул ─────────────────────────

/// RunLengthEncoder совпадает с наивным оракулом на случайных входах.
#[test]
fn prop_rle_matches_oracle() {
    let mut rng: u64 = 0xDEAD_BEEF_CAFE_1234;
    for _ in 0..1000 {
        let input = gen_input(&mut rng, 50);
        let got = rle(&input);
        let expected = naive_rle(&input);
        assert_eq!(got, expected, "расхождение с оракулом на входе {:?}", input);
    }
}

// ── Инвариант 2: декодирование восстанавливает вход ───────────────────────────

/// flat_map(повторить значение count раз) должен вернуть исходный вектор.
#[test]
fn prop_rle_decode_roundtrip() {
    let mut rng: u64 = 0x1234_5678_9ABC_DEF0;
    for _ in 0..1000 {
        let input = gen_input(&mut rng, 50);
        let encoded = rle(&input);
        let decoded: Vec<i64> = encoded
            .iter()
            .flat_map(|&(val, count)| std::iter::repeat(val).take(count))
            .collect();
        assert_eq!(decoded, input, "roundtrip не удался для {:?}", input);
    }
}

// ── Инвариант 3: сумма count == len(input) ────────────────────────────────────

/// Сумма всех count в парах равна длине входного вектора.
#[test]
fn prop_rle_total_count_equals_len() {
    let mut rng: u64 = 0xABCD_EF01_2345_6789;
    for _ in 0..1000 {
        let input = gen_input(&mut rng, 60);
        let pairs = rle(&input);
        let total: usize = pairs.iter().map(|(_, c)| c).sum();
        assert_eq!(
            total,
            input.len(),
            "суммарный count={total} != len={} для {:?}",
            input.len(),
            input
        );
    }
}

// ── Инвариант 4: count > 0 для каждой пары ───────────────────────────────────

/// Каждая пара должна иметь count >= 1 (пустые прогоны не допускаются).
#[test]
fn prop_rle_no_zero_counts() {
    let mut rng: u64 = 0xFEED_FACE_CAFE_BABE;
    for _ in 0..1000 {
        let input = gen_input(&mut rng, 50);
        let pairs = rle(&input);
        for &(val, count) in &pairs {
            assert!(
                count >= 1,
                "count=0 для значения {val} во входе {:?}",
                input
            );
        }
    }
}

// ── Инвариант 5: соседние пары имеют разные значения ─────────────────────────

/// В выходе не может быть двух подряд идущих пар с одинаковым значением.
/// (Если есть — это означает, что прогон не был слит в одну пару.)
#[test]
fn prop_rle_no_adjacent_same_values() {
    let mut rng: u64 = 0x0102_0304_0506_0708;
    for _ in 0..1000 {
        let input = gen_input(&mut rng, 50);
        let pairs = rle(&input);
        for w in pairs.windows(2) {
            assert_ne!(
                w[0].0, w[1].0,
                "соседние пары с одинаковым значением {} во входе {:?}",
                w[0].0,
                input
            );
        }
    }
}

// ── Инвариант 6: пустой вход → пустой выход ──────────────────────────────────

/// Для любого пустого итератора RunLengthEncoder тоже пуст.
#[test]
fn prop_rle_empty_in_empty_out() {
    for _ in 0..100 {
        let got = rle(&[]);
        assert!(got.is_empty());
    }
}

// ── Инвариант 7: однородный вход → ровно одна пара ───────────────────────────

/// Вектор из одного значения, повторённого N раз, даёт ровно [(value, N)].
#[test]
fn prop_rle_uniform_input_single_pair() {
    let mut rng: u64 = 0xCAFE_BABE_DEAD_BEEF;
    for _ in 0..500 {
        let val = in_range(&mut rng, -100, 100);
        let count = (next_u64(&mut rng) % 50 + 1) as usize;
        let input: Vec<i64> = vec![val; count];
        let pairs = rle(&input);
        assert_eq!(pairs, vec![(val, count)], "вход: {:?}", input);
    }
}
