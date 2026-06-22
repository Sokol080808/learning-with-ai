// Эти тесты трогать не нужно — это эталон поведения.
// Они описывают `concurrent_counter`, `atomic_counter` и `SharedTally` из src/lib.rs.
// Сейчас они падают (внутри `todo!()` — паника), но компилируются. Реализуй стаб —
// и красное станет зелёным.
//
// Здесь же — рандомизированные property-тесты на детерминированном ГПСЧ (xorshift64*),
// без внешних крейтов: прогон всегда воспроизводим.

use m15_concurrency::{atomic_counter, concurrent_counter, SharedTally};
use std::sync::Arc;
use std::thread;

// ---------------------------------------------------------------------------
// Детерминированный ГПСЧ (xorshift64*) — без внешних крейтов.
// ---------------------------------------------------------------------------

fn next_u64(state: &mut u64) -> u64 {
    let mut x = *state;
    x ^= x >> 12;
    x ^= x << 25;
    x ^= x >> 27;
    *state = x;
    x.wrapping_mul(0x2545F4914F6CDD1D)
}

/// целое в диапазоне [lo, hi] включительно.
fn in_range(state: &mut u64, lo: i64, hi: i64) -> i64 {
    let span = (hi - lo + 1) as u64;
    lo + (next_u64(state) % span) as i64
}

// ===========================================================================
// Задание 3 — concurrent_counter (Arc<Mutex<usize>>)
// ===========================================================================

#[test]
fn concurrent_counter_canonical_8x500() {
    // Главный экзамен: двойник капстоунного concurrent_lpush. Ни один инкремент не теряется.
    assert_eq!(concurrent_counter(8, 500), 4000);
}

#[test]
fn concurrent_counter_single_thread() {
    assert_eq!(concurrent_counter(1, 1000), 1000);
}

#[test]
fn concurrent_counter_zero_threads_is_zero() {
    assert_eq!(concurrent_counter(0, 100), 0);
}

#[test]
fn concurrent_counter_zero_increments_is_zero() {
    assert_eq!(concurrent_counter(4, 0), 0);
}

/// Оракул: concurrent_counter(t, i) == t * i.
#[test]
fn prop_concurrent_counter_matches_oracle() {
    let mut rng: u64 = 0xAAAA_BBBB_CCCC_DDDD;
    for _ in 0..200 {
        let threads = in_range(&mut rng, 1, 12) as usize;
        let increments = in_range(&mut rng, 0, 300) as usize;
        assert_eq!(
            concurrent_counter(threads, increments),
            threads * increments,
            "несовпадение при threads={threads}, increments={increments}"
        );
    }
}

// ===========================================================================
// Задание 4 — atomic_counter (Arc<AtomicUsize>)
// ===========================================================================

#[test]
fn atomic_counter_canonical_8x500() {
    assert_eq!(atomic_counter(8, 500), 4000);
}

#[test]
fn atomic_counter_single_thread() {
    assert_eq!(atomic_counter(1, 1000), 1000);
}

#[test]
fn atomic_counter_zero_threads_is_zero() {
    assert_eq!(atomic_counter(0, 100), 0);
}

#[test]
fn atomic_counter_zero_increments_is_zero() {
    assert_eq!(atomic_counter(4, 0), 0);
}

/// Паритет с concurrent_counter на одинаковых кейсах + случайные пары == t*i.
#[test]
fn prop_atomic_counter_parity_with_mutex() {
    let mut rng: u64 = 0x0102_0304_0506_0708;
    // фиксированные кейсы — паритет
    for &(t, i) in &[(8usize, 500usize), (1, 1000), (0, 100), (4, 0), (3, 250)] {
        assert_eq!(atomic_counter(t, i), concurrent_counter(t, i));
        assert_eq!(atomic_counter(t, i), t * i);
    }
    // случайные пары
    for _ in 0..200 {
        let threads = in_range(&mut rng, 1, 12) as usize;
        let increments = in_range(&mut rng, 0, 300) as usize;
        assert_eq!(
            atomic_counter(threads, increments),
            threads * increments,
            "несовпадение при threads={threads}, increments={increments}"
        );
    }
}

// ===========================================================================
// Задание 5 — SharedTally (Arc<Mutex<HashMap>>) — двойник капстоунного SharedDb
// ===========================================================================

/// handle() даёт ВТОРОЙ дескриптор того же хранилища, а не копию: запись через `a`
/// видна через `b`.
#[test]
fn shared_tally_handle_shares_same_store() {
    let a = SharedTally::new();
    let b = a.handle();
    a.add("x", 5);
    assert_eq!(b.get("x"), 5, "handle() должен указывать на ТЕ ЖЕ данные");
}

/// clone() эквивалентен handle() — тоже общий доступ к одному хранилищу.
#[test]
fn shared_tally_clone_shares_same_store() {
    let a = SharedTally::new();
    let b = a.clone();
    b.add("y", 7);
    assert_eq!(a.get("y"), 7, "clone() == handle(): то же хранилище");
}

/// add возвращает НОВОЕ накопленное значение; get несуществующего ключа == 0.
#[test]
fn shared_tally_add_returns_running_total() {
    let t = SharedTally::new();
    assert_eq!(t.add("k", 3), 3);
    assert_eq!(t.add("k", 4), 7);
    assert_eq!(t.get("k"), 7);
    assert_eq!(t.get("missing"), 0);
}

/// add умеет вычитать (отрицательная delta) и работать с несколькими ключами.
#[test]
fn shared_tally_independent_keys_and_negative_delta() {
    let t = SharedTally::new();
    t.add("a", 10);
    t.add("b", 100);
    assert_eq!(t.add("a", -3), 7);
    assert_eq!(t.get("a"), 7);
    assert_eq!(t.get("b"), 100);
}

/// default() == new().
#[test]
fn shared_tally_default_is_empty() {
    let t = SharedTally::default();
    assert_eq!(t.get("anything"), 0);
}

/// Компиляционная гарантия: SharedTally пригоден для потоков (Send + Sync).
/// Если бы внутри был Rc вместо Arc, этот тест не скомпилировался бы.
#[test]
fn shared_tally_is_send_and_sync() {
    fn assert_send_sync<T: Send + Sync>() {}
    assert_send_sync::<SharedTally>();
}

/// Главный конкурентный тест: 8 потоков, каждый делает 500 раз add("shared", 1)
/// через свой handle() → итог ровно 8 * 500 = 4000. Двойник капстоунного concurrent_lpush.
#[test]
fn shared_tally_concurrent_same_key() {
    const THREADS: usize = 8;
    const PER_THREAD: i64 = 500;

    let tally = SharedTally::new();
    let mut handles = Vec::new();
    for _ in 0..THREADS {
        let t = tally.handle(); // второй дескриптор того же хранилища
        let h = thread::spawn(move || {
            for _ in 0..PER_THREAD {
                t.add("shared", 1);
            }
        });
        handles.push(h);
    }
    for h in handles {
        h.join().expect("worker thread panicked");
    }

    assert_eq!(
        tally.get("shared"),
        THREADS as i64 * PER_THREAD,
        "ни один add не должен потеряться: ожидали {}",
        THREADS as i64 * PER_THREAD
    );
}

/// Независимые ключи: каждый из 4 потоков копит в СВОЙ ключ → значения не смешиваются.
#[test]
fn shared_tally_concurrent_independent_keys() {
    const THREADS: usize = 4;
    const PER_THREAD: i64 = 500;

    let tally = SharedTally::new();
    let mut handles = Vec::new();
    for tid in 0..THREADS {
        let t = tally.handle();
        let key = format!("key{tid}");
        let h = thread::spawn(move || {
            for _ in 0..PER_THREAD {
                t.add(&key, 1);
            }
        });
        handles.push(h);
    }
    for h in handles {
        h.join().expect("worker thread panicked");
    }

    for tid in 0..THREADS {
        assert_eq!(
            tally.get(&format!("key{tid}")),
            PER_THREAD,
            "ключ key{tid} должен накопить ровно {PER_THREAD}"
        );
    }
}

/// Свойство (рандомизированное): N потоков по K инкрементов в общий ключ → итог N*K,
/// при разных N и K. Проверяет, что Mutex сериализует доступ при любой нагрузке.
#[test]
fn prop_shared_tally_concurrent_total() {
    let mut rng: u64 = 0xFEED_FACE_DEAD_BEEF;
    for _ in 0..40 {
        let threads = in_range(&mut rng, 1, 8) as usize;
        let per_thread = in_range(&mut rng, 0, 300);

        let tally = SharedTally::new();
        // Arc вокруг tally не нужен — SharedTally сам клонируется в дескриптор (handle).
        let mut handles = Vec::new();
        for _ in 0..threads {
            let t = tally.handle();
            let h = thread::spawn(move || {
                for _ in 0..per_thread {
                    t.add("acc", 1);
                }
            });
            handles.push(h);
        }
        for h in handles {
            h.join().expect("worker thread panicked");
        }

        assert_eq!(
            tally.get("acc"),
            threads as i64 * per_thread,
            "итог должен быть threads*per_thread = {}*{}",
            threads,
            per_thread
        );
    }
}

/// Свойство: SharedTally можно положить в Arc и шарить иначе (демонстрация Send+Sync
/// в действии) — суммарный итог по случайным delta совпадает с наивным оракулом.
#[test]
fn prop_shared_tally_arc_wrapped_sum() {
    let mut rng: u64 = 0x1234_5678_ABCD_EF00;
    for _ in 0..40 {
        let threads = in_range(&mut rng, 1, 6) as usize;
        let ops = in_range(&mut rng, 0, 50) as usize;

        // Заранее генерируем deltas детерминированно и считаем оракул.
        let mut all_deltas: Vec<Vec<i64>> = Vec::new();
        let mut oracle: i64 = 0;
        for _ in 0..threads {
            let mut row = Vec::new();
            for _ in 0..ops {
                let d = in_range(&mut rng, -100, 100);
                oracle += d;
                row.push(d);
            }
            all_deltas.push(row);
        }

        let tally = Arc::new(SharedTally::new());
        let mut handles = Vec::new();
        for row in all_deltas {
            let t = Arc::clone(&tally);
            let h = thread::spawn(move || {
                for d in row {
                    t.add("total", d);
                }
            });
            handles.push(h);
        }
        for h in handles {
            h.join().expect("worker thread panicked");
        }

        assert_eq!(tally.get("total"), oracle, "сумма deltas должна совпасть с оракулом");
    }
}
