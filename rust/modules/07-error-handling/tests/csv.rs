// Тесты для parse_csv_record и CsvError (задание 5 модуля 07).
//
// Структура:
//  - детерминированные примеры: конкретные строки → ожидаемый результат / ошибка;
//  - рандомизированные property-тесты: инварианты и round-trip, xorshift64* ГПСЧ.
//
// КРИТИЧНО: никаких compile-time зависимостей от тел функций — все вызовы
// решения происходят внутри #[test]-функций (runtime). Тот же файл компилируется
// с todo!()-заглушками на ветке main (тесты падают, но собираются).

use m07_errors::{parse_csv_record, CsvError};

// ---------------------------------------------------------------------------
// Вспомогательный ГПСЧ (тот же xorshift64*, что и в props.rs)
// ---------------------------------------------------------------------------

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

// ---------------------------------------------------------------------------
// Детерминированные примеры — нормальный путь (Ok)
// ---------------------------------------------------------------------------

#[test]
fn csv_ok_single_field() {
    assert_eq!(parse_csv_record("42", 1), Ok(vec![42]));
}

#[test]
fn csv_ok_three_fields() {
    assert_eq!(parse_csv_record("1,2,3", 3), Ok(vec![1, 2, 3]));
}

#[test]
fn csv_ok_negative_values() {
    assert_eq!(parse_csv_record("-1,-2,-3", 3), Ok(vec![-1, -2, -3]));
}

#[test]
fn csv_ok_mixed_signs() {
    assert_eq!(parse_csv_record("-10,0,+5", 3), Ok(vec![-10, 0, 5]));
}

#[test]
fn csv_ok_spaces_around_fields() {
    // trim должен снять пробелы вокруг каждого поля
    assert_eq!(parse_csv_record(" 7 , -3 , 0 ", 3), Ok(vec![7, -3, 0]));
}

#[test]
fn csv_ok_large_value() {
    // i64::MAX / 2 — большое, но допустимое число
    let big = 4_611_686_018_427_387_903i64;
    let s = big.to_string();
    assert_eq!(parse_csv_record(&s, 1), Ok(vec![big]));
}

#[test]
fn csv_ok_zero_fields_expected() {
    // Пустая строка "" при split(',') даёт ровно 1 поле — пустое "".
    // Запрос 0 полей должен завершиться WrongColumnCount.
    // Но строка с ровно нулём запятых — то есть пустая строка — даёт 1 поле.
    // Проверяем граничный случай: если ожидаем 1 поле и передаём "0" — Ok.
    assert_eq!(parse_csv_record("0", 1), Ok(vec![0]));
}

// ---------------------------------------------------------------------------
// Детерминированные примеры — ошибка WrongColumnCount
// ---------------------------------------------------------------------------

#[test]
fn csv_err_wrong_count_too_few() {
    let result = parse_csv_record("1,2", 3);
    match result {
        Err(CsvError::WrongColumnCount { expected: 3, got: 2 }) => {}
        other => panic!("ожидалось WrongColumnCount{{3,2}}, получили {other:?}"),
    }
}

#[test]
fn csv_err_wrong_count_too_many() {
    let result = parse_csv_record("1,2,3,4", 3);
    match result {
        Err(CsvError::WrongColumnCount { expected: 3, got: 4 }) => {}
        other => panic!("ожидалось WrongColumnCount{{3,4}}, получили {other:?}"),
    }
}

#[test]
fn csv_err_wrong_count_single_expected_but_many_given() {
    let result = parse_csv_record("1,2", 1);
    match result {
        Err(CsvError::WrongColumnCount { expected: 1, got: 2 }) => {}
        other => panic!("ожидалось WrongColumnCount{{1,2}}, получили {other:?}"),
    }
}

#[test]
fn csv_err_wrong_count_empty_line_expected_three() {
    // "" при split(',') → одно поле (пустая строка), ожидаем 3
    let result = parse_csv_record("", 3);
    match result {
        Err(CsvError::WrongColumnCount { expected: 3, got: 1 }) => {}
        other => panic!("ожидалось WrongColumnCount{{3,1}}, получили {other:?}"),
    }
}

// ---------------------------------------------------------------------------
// Детерминированные примеры — ошибка BadField
// ---------------------------------------------------------------------------

#[test]
fn csv_err_bad_field_first_column() {
    // "abc" не является числом → BadField { col: 0, ... }
    let result = parse_csv_record("abc,2,3", 3);
    match result {
        Err(CsvError::BadField { col: 0, .. }) => {}
        other => panic!("ожидалось BadField{{col:0}}, получили {other:?}"),
    }
}

#[test]
fn csv_err_bad_field_middle_column() {
    // колонка 1 (нумерация с нуля) — "xyz"
    let result = parse_csv_record("1,xyz,3", 3);
    match result {
        Err(CsvError::BadField { col: 1, .. }) => {}
        other => panic!("ожидалось BadField{{col:1}}, получили {other:?}"),
    }
}

#[test]
fn csv_err_bad_field_last_column() {
    let result = parse_csv_record("1,2,bad", 3);
    match result {
        Err(CsvError::BadField { col: 2, .. }) => {}
        other => panic!("ожидалось BadField{{col:2}}, получили {other:?}"),
    }
}

#[test]
fn csv_err_bad_field_float_not_int() {
    // 1.5 не является i64 — должна быть ошибка разбора
    let result = parse_csv_record("1,1.5,3", 3);
    match result {
        Err(CsvError::BadField { col: 1, .. }) => {}
        other => panic!("ожидалось BadField{{col:1}} для '1.5', получили {other:?}"),
    }
}

#[test]
fn csv_err_wrong_count_checked_before_bad_field() {
    // "abc" — плохое поле, но сначала должна сработать проверка количества (2 vs 3)
    let result = parse_csv_record("abc,2", 3);
    match result {
        Err(CsvError::WrongColumnCount { .. }) => {}
        other => panic!("ожидалось WrongColumnCount (проверка числа полей идёт первой), получили {other:?}"),
    }
}

// ---------------------------------------------------------------------------
// Display: проверяем, что сообщения содержат ключевые данные
// ---------------------------------------------------------------------------

#[test]
fn csv_error_display_wrong_column_count_contains_numbers() {
    let result = parse_csv_record("1,2", 5);
    let msg = result.unwrap_err().to_string();
    assert!(
        msg.contains('5') && msg.contains('2'),
        "Display для WrongColumnCount должен содержать 5 и 2, но: '{msg}'"
    );
}

#[test]
fn csv_error_display_bad_field_contains_col_index() {
    let result = parse_csv_record("1,oops,3", 3);
    let msg = result.unwrap_err().to_string();
    assert!(
        msg.contains('1'),
        "Display для BadField должен содержать номер колонки (1), но: '{msg}'"
    );
}

// ---------------------------------------------------------------------------
// Property-тесты: инварианты и round-trip
// ---------------------------------------------------------------------------

/// Если вектор чисел сериализовать в CSV-строку, parse_csv_record должен
/// вернуть тот же вектор (round-trip).
#[test]
fn prop_round_trip_valid_csv() {
    let mut rng: u64 = 0xABCD_1234_EF56_7890;
    for _ in 0..800 {
        let len = in_range(&mut rng, 1, 8) as usize;
        // Держим числа скромными, чтобы форматирование было простым
        let values: Vec<i64> = (0..len)
            .map(|_| in_range(&mut rng, -9999, 9999))
            .collect();
        let line = values
            .iter()
            .map(|v| v.to_string())
            .collect::<Vec<_>>()
            .join(",");

        let result = parse_csv_record(&line, len);
        assert_eq!(
            result,
            Ok(values.clone()),
            "round-trip failed for values={values:?}, line='{line}'"
        );
    }
}

/// Строка с неправильным числом полей всегда даёт WrongColumnCount.
#[test]
fn prop_wrong_count_always_err() {
    let mut rng: u64 = 0x1111_AAAA_2222_BBBB;
    for _ in 0..600 {
        let actual_cols = in_range(&mut rng, 1, 6) as usize;
        let expected_cols = {
            // гарантируем, что expected != actual
            let candidate = in_range(&mut rng, 1, 6) as usize;
            if candidate == actual_cols {
                // сдвигаем на 1, оставаясь в диапазоне 1..=7
                if actual_cols < 7 { actual_cols + 1 } else { actual_cols - 1 }
            } else {
                candidate
            }
        };
        // строим строку из actual_cols числовых полей
        let fields: Vec<String> = (0..actual_cols)
            .map(|_| in_range(&mut rng, -100, 100).to_string())
            .collect();
        let line = fields.join(",");

        let result = parse_csv_record(&line, expected_cols);
        match result {
            Err(CsvError::WrongColumnCount { expected, got }) => {
                assert_eq!(expected, expected_cols);
                assert_eq!(got, actual_cols);
            }
            other => panic!(
                "ожидалось WrongColumnCount, получили {other:?} \
                 (actual_cols={actual_cols}, expected_cols={expected_cols}, line='{line}')"
            ),
        }
    }
}

/// BadField содержит правильный номер колонки (тот, где реально плохое поле).
#[test]
fn prop_bad_field_col_index_correct() {
    let mut rng: u64 = 0x7890_ABCD_1234_EF56;
    for _ in 0..600 {
        let len = in_range(&mut rng, 1, 6) as usize;
        let bad_col = in_range(&mut rng, 0, (len - 1) as i64) as usize;

        let mut fields: Vec<String> = (0..len)
            .map(|_| in_range(&mut rng, -999, 999).to_string())
            .collect();
        fields[bad_col] = "not_a_number".to_string();

        let line = fields.join(",");
        let result = parse_csv_record(&line, len);
        match result {
            Err(CsvError::BadField { col, .. }) => {
                assert_eq!(
                    col, bad_col,
                    "неверный номер колонки: ожидалось {bad_col}, получили {col} \
                     (line='{line}')"
                );
            }
            other => panic!(
                "ожидалось BadField, получили {other:?} (line='{line}')"
            ),
        }
    }
}

/// WrongColumnCount проверяется ДО BadField — если строка одновременно имеет
/// неверное количество полей и плохие поля, должен победить WrongColumnCount.
#[test]
fn prop_wrong_count_before_bad_field() {
    let mut rng: u64 = 0xDEAD_BEEF_0102_0304;
    for _ in 0..400 {
        // строка с actual_cols числовых полей + одно "bad" поле = actual_cols+1 полей
        let actual_cols = in_range(&mut rng, 1, 5) as usize;
        let fields: Vec<String> = (0..actual_cols)
            .map(|_| in_range(&mut rng, -100, 100).to_string())
            .collect();
        // добавляем нечисловое поле, теперь полей actual_cols+1
        let mut all = fields;
        all.push("BADTOKEN".to_string());
        let line = all.join(",");
        let total_got = actual_cols + 1;

        // просим меньше полей, чем есть → должен сработать WrongColumnCount
        let expected = in_range(&mut rng, 1, actual_cols as i64) as usize;
        let result = parse_csv_record(&line, expected);
        match result {
            Err(CsvError::WrongColumnCount { got, .. }) => {
                assert_eq!(got, total_got);
            }
            other => panic!(
                "ожидалось WrongColumnCount, получили {other:?} \
                 (line='{line}', expected={expected})"
            ),
        }
    }
}

/// Все поля в Ok-результате попарно равны числам из исходного вектора (oracle).
#[test]
fn prop_ok_values_match_oracle() {
    let mut rng: u64 = 0x5A5A_A5A5_F0F0_0F0F;
    for _ in 0..500 {
        let len = in_range(&mut rng, 1, 10) as usize;
        let values: Vec<i64> = (0..len)
            .map(|_| in_range(&mut rng, -50_000, 50_000))
            .collect();
        let line = values
            .iter()
            .map(|v| v.to_string())
            .collect::<Vec<_>>()
            .join(",");

        match parse_csv_record(&line, len) {
            Ok(got) => {
                assert_eq!(got.len(), len, "длина результата должна равняться len={len}");
                for (i, (&expected, &actual)) in values.iter().zip(got.iter()).enumerate() {
                    assert_eq!(
                        actual, expected,
                        "поле {i}: ожидалось {expected}, получили {actual}"
                    );
                }
            }
            Err(e) => panic!("ожидалось Ok, получили Err({e}) для line='{line}'"),
        }
    }
}
