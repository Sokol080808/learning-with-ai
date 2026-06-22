// ВНИМАНИЕ: здесь пишешь ТЫ. Реализуй так, чтобы тесты модуля 15 стали зелёными.
//
// Модуль 15 — Конкурентность: потоки, каналы, Arc<Mutex>, атомики.
// Замени каждый `todo!()` настоящей реализацией. Типы, поля и сигнатуры уже объявлены —
// менять их не нужно (тесты на них рассчитывают). Стаб компилируется, но тесты падают
// (паника `not yet implemented`), пока тела помечены `todo!()`.

use std::collections::HashMap;
use std::sync::atomic::{AtomicUsize, Ordering};
use std::sync::mpsc;
use std::sync::{Arc, Mutex};
use std::thread;

// =============================================================================
// Задание 1 — параллельная сумма: thread::spawn + JoinHandle::join
// =============================================================================

/// Сумма `data`, посчитанная параллельно в `chunks` потоках.
///
/// Контракт:
/// - разбей `data` на ~равные `chunks` непрерывных частей;
/// - каждую часть просуммируй в своём `thread::spawn` (тело — `move`-замыкание,
///   забирающее владение куском);
/// - собери частичные суммы через `join()` и сложи их;
/// - результат обязан совпасть с `data.iter().sum()` и НЕ зависеть от числа частей.
/// - `chunks == 0` трактуй как `1` (нельзя делить на ноль).
///
/// ПОЧЕМУ `move`: тело потока обязано быть `'static` — оно не может заимствовать
/// локальный `data`, поэтому забирает кусок во владение. ПОЧЕМУ `join`: без него
/// `main` может завершиться раньше потоков, и часть работы потеряется.
pub fn parallel_sum(data: Vec<i64>, chunks: usize) -> i64 {
    let chunks = chunks.max(1);
    // Размер части округляем вверх, чтобы все элементы попали ровно в одну часть.
    let n = data.len();
    let chunk_size = n.div_ceil(chunks).max(1);

    let mut handles = Vec::new();
    for part in data.chunks(chunk_size) {
        // Забираем кусок во владение потока: тело потока `'static`, заимствовать `data` нельзя.
        let owned: Vec<i64> = part.to_vec();
        let handle = thread::spawn(move || owned.iter().sum::<i64>());
        handles.push(handle);
    }

    // Собираем частичные суммы. `join()` блокирует и отдаёт `Result` (Err = поток запаниковал).
    handles
        .into_iter()
        .map(|h| h.join().expect("worker thread panicked"))
        .sum()
}

// =============================================================================
// Задание 2 — конвейер через канал: mpsc, tx.clone(), drop(tx)
// =============================================================================

/// Возвести каждый вход в квадрат, раздав работу `workers` потокам через канал.
///
/// Контракт:
/// - создай канал `mpsc::channel()`;
/// - раздай элементы `inputs` примерно поровну между `workers` потоками;
/// - каждый поток отправляет `x * x` через СВОЙ клон `tx` (`tx.clone()`);
/// - главный поток собирает все значения из `rx` в `Vec`;
/// - `workers == 0` трактуй как `1`.
/// - КОНТРАКТ НА РЕЗУЛЬТАТ: мультимножество результата == `{ x*x | x in inputs }`.
///   Порядок не гарантирован (потоки финишируют недетерминированно), длина сохраняется.
///
/// ПОДВОХ: цикл `for v in rx` завершится, только когда сдропнуты ВСЕ `Sender`. Исходный
/// `tx` надо дропнуть (или не клонировать в главный поток), иначе `rx` будет ждать вечно —
/// учебный дедлок.
pub fn pipeline_collect(inputs: Vec<i64>, workers: usize) -> Vec<i64> {
    let workers = workers.max(1);
    let (tx, rx) = mpsc::channel::<i64>();

    // Раздаём входы по потокам: каждый поток забирает свою долю во владение.
    let n = inputs.len();
    let chunk_size = n.div_ceil(workers).max(1);

    let mut handles = Vec::new();
    for part in inputs.chunks(chunk_size) {
        let owned: Vec<i64> = part.to_vec();
        let tx = tx.clone(); // у каждого потока — свой клон отправителя
        let handle = thread::spawn(move || {
            for x in owned {
                // send ПЕРЕМЕЩАЕТ значение в канал → нет разделяемой изменяемой памяти.
                tx.send(x * x).expect("receiver hung up");
            }
            // tx (клон) дропается здесь, на выходе из замыкания.
        });
        handles.push(handle);
    }

    // КЛЮЧЕВОЕ: дропаем исходный tx. Иначе канал никогда не закроется и цикл ниже зависнет.
    drop(tx);

    // `for v in rx` крутится, пока жив хоть один Sender; завершится, когда сдропнут последний.
    let mut out = Vec::with_capacity(n);
    for v in rx {
        out.push(v);
    }

    // Дожидаемся потоков (на случай паники внутри — пробросим её наружу).
    for h in handles {
        h.join().expect("worker thread panicked");
    }
    out
}

// =============================================================================
// Задание 3 — общий счётчик через Arc<Mutex<usize>>
// =============================================================================

/// Общий счётчик: `threads` потоков, каждый `increments` раз делает `+= 1`.
///
/// Контракт: вернуть итоговое значение, которое ОБЯЗАНО быть равно
/// `threads * increments` — ни один инкремент не теряется.
///
/// Рецепт: общий `Arc<Mutex<usize>>`; в каждый поток передать `Arc::clone`; внутри потока
/// `increments` раз взять `lock()` и увеличить. `Arc` даёт нескольким потокам один счётчик;
/// `Mutex` выстраивает их в очередь, так что гонки данных невозможны.
///
/// Это однопоточный двойник капстоуна (`concurrent_lpush`: 8 потоков × 500 = 4000).
pub fn concurrent_counter(threads: usize, increments: usize) -> usize {
    // Arc — разделяемое владение между потоками; Mutex — эксклюзивный доступ одному за раз.
    let counter = Arc::new(Mutex::new(0usize));

    let mut handles = Vec::new();
    for _ in 0..threads {
        let counter = Arc::clone(&counter); // дёшево: атомарный инкремент счётчика ссылок
        let handle = thread::spawn(move || {
            for _ in 0..increments {
                // Держим guard коротко: блок { } гарантирует, что замок отпускается сразу.
                let mut guard = counter.lock().expect("mutex poisoned");
                *guard += 1;
            }
        });
        handles.push(handle);
    }

    for h in handles {
        h.join().expect("worker thread panicked");
    }

    // Все потоки завершились → можно безопасно прочитать итог.
    let final_value = *counter.lock().expect("mutex poisoned");
    final_value
}

// =============================================================================
// Задание 4 — тот же счётчик через Arc<AtomicUsize> (контраст-урок)
// =============================================================================

/// То же, что [`concurrent_counter`], но через атомик вместо мьютекса.
///
/// Контракт: результат == `threads * increments`.
///
/// Для простого счётчика `Mutex` избыточен: `AtomicUsize::fetch_add` — одна атомарная
/// инструкция процессора, без блокировки и без guard'а. `Atomic*` сами по себе `Sync`,
/// поэтому достаточно `Arc<AtomicUsize>` — `Mutex` не нужен.
///
/// `Ordering::Relaxed` здесь корректен: нам важна только атомарность каждого `+1`, а не
/// упорядочивание относительно других переменных. (Глубокая memory ordering — вне junior;
/// если сомневаешься, безопасный дефолт — `SeqCst`.)
pub fn atomic_counter(threads: usize, increments: usize) -> usize {
    let counter = Arc::new(AtomicUsize::new(0));

    let mut handles = Vec::new();
    for _ in 0..threads {
        let counter = Arc::clone(&counter);
        let handle = thread::spawn(move || {
            for _ in 0..increments {
                counter.fetch_add(1, Ordering::Relaxed);
            }
        });
        handles.push(handle);
    }

    for h in handles {
        h.join().expect("worker thread panicked");
    }

    counter.load(Ordering::Relaxed)
}

// =============================================================================
// Задание 5 — SharedTally: мини-двойник капстоунного SharedDb
// =============================================================================

/// Потокобезопасный общий счётчик по ключам — учебная миниатюра капстоунного `SharedDb`.
///
/// Внутри — ровно тот же рецепт, что в `rust/capstone/src/shared.rs`:
///
/// ```text
/// Arc<Mutex<HashMap<String, i64>>>
///  │     │      └── сами данные (накопленные суммы по ключам)
///  │     └── Mutex: к данным в любой момент имеет доступ только ОДИН поток
///  └── Arc: разделяемое владение — несколько потоков «держат» одно хранилище
/// ```
///
/// Клонируется дёшево (через [`SharedTally::handle`]) и передаётся в потоки. Все клоны
/// делят ОДНУ и ту же `HashMap` под одним `Mutex` — это не копия данных, а второй
/// дескриптор того же хранилища.
///
/// Поскольку внутри `Arc<Mutex<…>>`, тип автоматически `Send + Sync` — компилятор сам это
/// выводит, и `thread::spawn` принимает `SharedTally` без возражений (см. тест с witness
/// `assert_send_sync`).
pub struct SharedTally {
    // Arc — разделяемое владение между потоками; Mutex — взаимное исключение доступа;
    // HashMap — собственно данные (ключ → накопленная сумма).
    inner: Arc<Mutex<HashMap<String, i64>>>,
}

impl SharedTally {
    /// Создать новое пустое хранилище (внутри — свежая пустая `HashMap`).
    ///
    /// Реализуй как `Arc::new(Mutex::new(HashMap::new()))`, завёрнутое в `SharedTally`.
    pub fn new() -> Self {
        SharedTally {
            inner: Arc::new(Mutex::new(HashMap::new())),
        }
    }

    /// Получить ещё один дескриптор ТОГО ЖЕ хранилища — для передачи в другой поток.
    ///
    /// Контракт: возвращает `SharedTally`, указывающую на те же данные (через
    /// `Arc::clone(&self.inner)`), а НЕ на копию. Данные не дублируются — растёт лишь
    /// счётчик ссылок. Это ключ к тому, чтобы N потоков работали с ОДНИМ хранилищем.
    ///
    /// (По сути то же, что `self.clone()`, но имя `handle` явно говорит о намерении:
    /// «дай мне ручку к тому же хранилищу для другого потока».)
    pub fn handle(&self) -> SharedTally {
        SharedTally {
            inner: Arc::clone(&self.inner),
        }
    }

    /// Прибавить `delta` к значению по ключу `key` и вернуть НОВОЕ значение.
    ///
    /// Контракт:
    /// - захватить `Mutex` (`self.inner.lock()`), получив эксклюзивный `&mut HashMap`;
    /// - `entry(key).or_insert(0)` — взять (или создать с нулём) запись и прибавить `delta`;
    /// - вернуть получившееся значение;
    /// - замок отпускается автоматически, когда guard выходит из области видимости (RAII).
    ///
    /// `lock()` возвращает `Result` (Err только при отравленном мьютексе — держатель
    /// запаниковал). В учебном задании `.unwrap()` / `.expect(...)` допустимы.
    pub fn add(&self, key: &str, delta: i64) -> i64 {
        let mut guard = self.inner.lock().expect("mutex poisoned");
        let slot = guard.entry(key.to_string()).or_insert(0);
        *slot += delta;
        *slot
    }

    /// Текущее значение по ключу `key`, или `0`, если ключа ещё нет.
    ///
    /// Контракт: захватить замок, вернуть значение из `HashMap` (или `0`).
    pub fn get(&self, key: &str) -> i64 {
        let guard = self.inner.lock().expect("mutex poisoned");
        guard.get(key).copied().unwrap_or(0)
    }
}

// `SharedTally::default()` == `SharedTally::new()`.
impl Default for SharedTally {
    fn default() -> Self {
        Self::new()
    }
}

// Клонирование `SharedTally` == получить новый дескриптор того же хранилища.
// Реализовано через `handle()`, чтобы `.clone()` и `.handle()` были эквивалентны.
impl Clone for SharedTally {
    fn clone(&self) -> Self {
        self.handle()
    }
}
