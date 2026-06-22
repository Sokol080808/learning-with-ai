// Тесты для задания 5: Config (синтаксис обновления структуры) и Sentinel (юнит-тип).
// Запуск: ./rust/run.sh 05
//
// Это отдельный интеграционный крейт — импортируем ТОЛЬКО публичное API m05_structs.

use m05_structs::*;

// маленький детерминированный ГПСЧ (xorshift64*), без внешних крейтов — воспроизводимо.
// Сид ФИКСИРОВАННЫЙ (hardcode), чтобы прогон был детерминированным.
fn next_u64(state: &mut u64) -> u64 {
    let mut x = *state;
    x ^= x >> 12;
    x ^= x << 25;
    x ^= x >> 27;
    *state = x;
    x.wrapping_mul(0x2545F4914F6CDD1D)
}
// целое в диапазоне [lo, hi] включительно.
fn in_range(state: &mut u64, lo: i64, hi: i64) -> i64 {
    let span = (hi - lo + 1) as u64;
    lo + (next_u64(state) % span) as i64
}

// -----------------------------------------------------------------------
// Config::new — детерминированные тесты
// -----------------------------------------------------------------------

#[test]
fn config_new_sets_name() {
    let c = Config::new("db");
    assert_eq!(c.name, "db", "Config::new должен сохранить переданное имя");
}

// -----------------------------------------------------------------------
// Config::with_timeout — меняет ТОЛЬКО timeout, остальное не трогает
// -----------------------------------------------------------------------

#[test]
fn with_timeout_changes_only_timeout() {
    let base = Config::new("server");
    let fast = base.with_timeout(5);

    // изменилось только поле timeout:
    assert_eq!(fast.timeout, 5, "with_timeout должен выставить новый timeout");

    // все остальные поля совпадают с исходными:
    assert_eq!(fast.name, base.name, "name не должен меняться");
    assert_eq!(fast.retries, base.retries, "retries не должен меняться");
    assert_eq!(fast.verbose, base.verbose, "verbose не должен меняться");
}

#[test]
fn with_timeout_does_not_mutate_original() {
    // with_timeout берёт &self → исходный конфиг остаётся доступен и неизменён.
    let base = Config::new("orig");
    let original_timeout = base.timeout;
    let _new = base.with_timeout(original_timeout.wrapping_add(99));

    // base всё ещё наш и его timeout не изменился:
    assert_eq!(
        base.timeout, original_timeout,
        "with_timeout не должен мутировать исходный Config"
    );
    assert_eq!(base.name, "orig");
}

#[test]
fn with_timeout_is_idempotent_on_same_value() {
    // Если попросить тот же timeout, результат должен быть равен исходнику (PartialEq).
    let base = Config::new("idem");
    let same = base.with_timeout(base.timeout);
    assert_eq!(same, base, "with_timeout(текущий timeout) == исходный Config");
}

// -----------------------------------------------------------------------
// Sentinel — юнит-структура с методом
// -----------------------------------------------------------------------

#[test]
fn sentinel_is_active_is_true() {
    let s = Sentinel;
    assert!(s.is_active(), "Sentinel.is_active() должен возвращать true");
}

#[test]
fn sentinel_is_zero_sized() {
    // Юнит-структура занимает 0 байт.
    assert_eq!(std::mem::size_of::<Sentinel>(), 0, "Sentinel должен быть нулевого размера");
}

// -----------------------------------------------------------------------
// {:#?} — pretty-print через Debug должен содержать имена полей
// -----------------------------------------------------------------------

#[test]
fn pretty_debug_contains_field_names() {
    let c = Config::new("pretty");
    let pretty = format!("{c:#?}");

    // вывод не пустой
    assert!(!pretty.is_empty(), "{{:#?}} вывод не должен быть пустым");

    // pretty-форма содержит имена полей структуры
    assert!(pretty.contains("name"), "{{:#?}} должен содержать поле name, got: {pretty}");
    assert!(pretty.contains("timeout"), "{{:#?}} должен содержать поле timeout, got: {pretty}");

    // и многострочный: pretty-форма переносит каждое поле на свою строку
    assert!(
        pretty.contains('\n'),
        "{{:#?}} должен быть многострочным (с переносами), got: {pretty}"
    );
}

#[test]
fn compact_and_pretty_differ() {
    let c = Config::new("fmt");
    let compact = format!("{c:?}");
    let pretty = format!("{c:#?}");
    // компактная форма — в одну строку, pretty — с переносами; они различаются.
    assert_ne!(compact, pretty, "{{:?}} и {{:#?}} должны давать разный вывод");
    assert!(!compact.contains('\n'), "{{:?}} должен быть однострочным");
}

// -----------------------------------------------------------------------
// Property-тест: with_timeout сохраняет все поля кроме timeout для случайных входов
// -----------------------------------------------------------------------

#[test]
fn prop_with_timeout_preserves_other_fields() {
    let mut rng: u64 = 0x5715_C0FF_EE15_600D;
    let names = ["a", "service", "db-primary", "cache", ""];
    for _ in 0..1000 {
        // случайный исходный конфиг: имя из набора + случайный стартовый timeout
        let name = names[(next_u64(&mut rng) % names.len() as u64) as usize];
        let start_timeout = in_range(&mut rng, 0, 100_000) as u32;
        let base = Config::new(name).with_timeout(start_timeout);

        // случайный новый timeout
        let new_timeout = in_range(&mut rng, 0, 100_000) as u32;
        let updated = base.with_timeout(new_timeout);

        // изменилось только timeout:
        assert_eq!(updated.timeout, new_timeout, "timeout должен стать новым значением");
        // все прочие поля идентичны базовым:
        assert_eq!(updated.name, base.name, "name изменился у {base:?}");
        assert_eq!(updated.retries, base.retries, "retries изменился у {base:?}");
        assert_eq!(updated.verbose, base.verbose, "verbose изменился у {base:?}");
    }
}
