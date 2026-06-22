// Тесты для задания 6 — CachedValue: кэш поверх слабой ссылки (Weak + Cell).
// Это собственный интеграционный крейт: импортируем только публичный API модуля 13.
//
// Контракт (см. README, «Задание 6»):
//   - пока источник жив, refresh() возвращает Some(value) и обновляет cached();
//   - после дропа последнего сильного Rc, refresh() возвращает None,
//     а cached() всё ещё отдаёт ПОСЛЕДНЕЕ закэшированное значение;
//   - мутация кэша идёт через &self (внутренняя изменяемость через Cell).

use std::cell::RefCell;
use std::rc::Rc;

use m13_smart_pointers::*;

// ---------------------------------------------------------------------------
// Детерминированный ГПСЧ (xorshift64*) — без внешних крейтов.
// Сид ФИКСИРОВАННЫЙ, прогон детерминирован (тот же helper, что и в props.rs).
// ---------------------------------------------------------------------------

fn next_u64(state: &mut u64) -> u64 {
    let mut x = *state;
    x ^= x >> 12;
    x ^= x << 25;
    x ^= x >> 27;
    *state = x;
    x.wrapping_mul(0x2545F4914F6CDD1D)
}

/// целое в диапазоне [lo, hi] включительно
fn in_range(state: &mut u64, lo: i64, hi: i64) -> i64 {
    let span = (hi - lo + 1) as u64;
    lo + (next_u64(state) % span) as i64
}

// ===========================================================================
// Детерминированные примеры
// ===========================================================================

/// Сразу после new() кэш пуст (== 0), пока refresh не вызван.
#[test]
fn new_starts_with_zero_cache() {
    let source = Rc::new(RefCell::new(123));
    let cached = CachedValue::new(&source);
    assert_eq!(cached.cached(), 0);
}

/// Пока источник жив, refresh() возвращает Some(текущее значение) и кладёт его в кэш.
#[test]
fn refresh_while_alive_returns_some_and_updates_cache() {
    let source = Rc::new(RefCell::new(42));
    let cached = CachedValue::new(&source);

    assert_eq!(cached.refresh(), Some(42));
    assert_eq!(cached.cached(), 42);
}

/// refresh() видит изменения источника: меняем источник → refresh подхватывает.
#[test]
fn refresh_tracks_source_mutations() {
    let source = Rc::new(RefCell::new(10));
    let cached = CachedValue::new(&source);

    assert_eq!(cached.refresh(), Some(10));
    assert_eq!(cached.cached(), 10);

    *source.borrow_mut() = 99;
    // Кэш ещё не обновлён — он хранит ПРЕДЫДУЩЕЕ значение.
    assert_eq!(cached.cached(), 10);

    // А после refresh — новое.
    assert_eq!(cached.refresh(), Some(99));
    assert_eq!(cached.cached(), 99);
}

/// После дропа источника refresh() == None, а cached() хранит последнее значение.
#[test]
fn refresh_after_drop_returns_none_and_keeps_last_cache() {
    let source = Rc::new(RefCell::new(7));
    let cached = CachedValue::new(&source);

    assert_eq!(cached.refresh(), Some(7));
    assert_eq!(cached.cached(), 7);

    // Роняем единственный сильный Rc — источник освобождается.
    drop(source);

    // Слабая ссылка протухла: upgrade() == None → refresh() == None.
    assert_eq!(cached.refresh(), None);
    // Но последний кэш независим от источника — он сохранился.
    assert_eq!(cached.cached(), 7);
}

/// Если refresh ни разу не успел до дропа, кэш остаётся стартовым нулём,
/// а refresh после дропа == None.
#[test]
fn refresh_after_drop_without_prior_refresh_is_none_and_zero() {
    let source = Rc::new(RefCell::new(555));
    let cached = CachedValue::new(&source);

    drop(source);

    assert_eq!(cached.refresh(), None);
    assert_eq!(cached.cached(), 0);
}

/// Мутация кэша идёт через ОБЩУЮ ссылку &self — это и есть внутренняя изменяемость.
#[test]
fn refresh_mutates_through_shared_reference() {
    let source = Rc::new(RefCell::new(3));
    let cached = CachedValue::new(&source);
    let view: &CachedValue = &cached; // обычная неизменяемая ссылка

    assert_eq!(view.refresh(), Some(3));
    assert_eq!(view.cached(), 3);
}

/// Слабая ссылка НЕ удерживает источник в живых: пока кэш существует,
/// strong_count источника не растёт (создание кэша = downgrade, не clone).
#[test]
fn cache_does_not_extend_source_lifetime() {
    let source = Rc::new(RefCell::new(1));
    assert_eq!(Rc::strong_count(&source), 1);

    let _cached = CachedValue::new(&source);
    // CachedValue держит лишь Weak — счётчик сильных ссылок остаётся 1.
    assert_eq!(Rc::strong_count(&source), 1);
}

/// Несколько успешных refresh подряд оставляют в кэше ПОСЛЕДНЕЕ значение.
#[test]
fn multiple_refresh_keeps_latest() {
    let source = Rc::new(RefCell::new(0));
    let cached = CachedValue::new(&source);

    for v in [1, 2, 3, 100, -5] {
        *source.borrow_mut() = v;
        assert_eq!(cached.refresh(), Some(v));
        assert_eq!(cached.cached(), v);
    }
}

// ===========================================================================
// Рандомизированные property-тесты (детерминированный сид)
// ===========================================================================

/// Свойство: пока источник жив, refresh() == Some(текущего значения источника)
/// и cached() после него совпадает с этим значением.
#[test]
fn prop_refresh_alive_matches_source() {
    let mut rng: u64 = 0xCACE_0001_FEED_BEEF;
    for _ in 0..1000 {
        let v = in_range(&mut rng, -1_000_000, 1_000_000);
        let source = Rc::new(RefCell::new(v));
        let cached = CachedValue::new(&source);

        assert_eq!(cached.refresh(), Some(v), "refresh должен вернуть Some({v})");
        assert_eq!(cached.cached(), v, "cached() должен совпасть с источником {v}");
    }
}

/// Свойство: после дропа источника refresh() всегда None, а cached() хранит
/// значение из ПОСЛЕДНЕГО успешного refresh (или 0, если его не было).
#[test]
fn prop_after_drop_none_and_cache_frozen() {
    let mut rng: u64 = 0xDEAD_F00D_0BAD_C0DE;
    for _ in 0..1000 {
        let v = in_range(&mut rng, -1_000_000, 1_000_000);
        let source = Rc::new(RefCell::new(v));
        let cached = CachedValue::new(&source);

        // С вероятностью ~50% делаем refresh ДО дропа.
        let did_refresh = next_u64(&mut rng) & 1 == 0;
        let expected_cache = if did_refresh {
            assert_eq!(cached.refresh(), Some(v));
            v
        } else {
            0 // refresh не делали — кэш стартовый
        };

        drop(source);

        // Источник мёртв: refresh обязан вернуть None и не трогать кэш.
        assert_eq!(cached.refresh(), None, "после дропа refresh() должен быть None");
        assert_eq!(
            cached.cached(),
            expected_cache,
            "кэш после дропа должен остаться {expected_cache}"
        );
    }
}

/// Свойство: последовательность изменений источника + refresh оставляет в кэше
/// именно последнее прочитанное значение.
#[test]
fn prop_cache_holds_last_refreshed_value() {
    let mut rng: u64 = 0x0BEE_F00D_1234_ABCD;
    for _ in 0..500 {
        let source = Rc::new(RefCell::new(0));
        let cached = CachedValue::new(&source);

        let steps = in_range(&mut rng, 1, 20) as usize;
        let mut last = 0;
        for _ in 0..steps {
            let v = in_range(&mut rng, -1_000_000, 1_000_000);
            *source.borrow_mut() = v;
            assert_eq!(cached.refresh(), Some(v));
            last = v;
        }
        assert_eq!(cached.cached(), last, "кэш должен хранить последнее значение {last}");
    }
}
