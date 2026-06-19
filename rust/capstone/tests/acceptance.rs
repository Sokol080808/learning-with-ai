//! Приёмочные тесты капстоуна — ЭТАЛОН ПОВЕДЕНИЯ. Их трогать не нужно.
//!
//! Это спецификация в исполняемом виде: то, что здесь написано, и есть «ТЗ» на minidb.
//! Сейчас тесты КОМПИЛИРУЮТСЯ, но падают — все тела в библиотеке помечены `todo!()`
//! (паника `not yet implemented`). Реализуй методы [`minidb::Database`], функцию
//! [`minidb::execute`] и обёртку [`minidb::SharedDb`] — и красное станет зелёным.
//!
//! Тесты ДЕТЕРМИНИРОВАНЫ: без сети и без зависимости от внешнего окружения; файлы для
//! персистентности создаются во временном каталоге (`std::env::temp_dir()`) с уникальным
//! именем, чтобы параллельный запуск тестов не мешал сам себе. Конкурентность проверяется
//! фиксированным числом потоков и операций — итоговая длина списка обязана совпадать с
//! суммой, если `Mutex` действительно сериализует доступ.
//!
//! Запуск: `cargo test -p minidb` (или `./rust/run.sh`, чтобы прогнать весь воркспейс).

use minidb::*;
use std::collections::HashMap;
use std::sync::atomic::{AtomicU64, Ordering};

// =============================================================================
// Майлстоун 1 — типизированное хранилище: строки, списки, хеши
// =============================================================================

// --- Строки: set / get ---

#[test]
fn set_then_get_returns_value() {
    let mut db = Database::new();
    db.set("name", "alice".to_string());
    assert_eq!(db.get("name"), Some("alice".to_string()));
}

#[test]
fn get_missing_key_is_none() {
    let db = Database::new();
    assert_eq!(db.get("nope"), None);
}

#[test]
fn set_overwrites_previous_value() {
    let mut db = Database::new();
    db.set("k", "first".to_string());
    db.set("k", "second".to_string());
    assert_eq!(db.get("k"), Some("second".to_string()));
}

// --- Value: enum-вариант напрямую ---

#[test]
fn value_variants_are_distinct() {
    let s = Value::Str("x".to_string());
    let l = Value::List(vec!["a".to_string()]);
    let mut m = HashMap::new();
    m.insert("f".to_string(), "v".to_string());
    let h = Value::Hash(m);

    assert_eq!(s.type_name(), "string");
    assert_eq!(l.type_name(), "list");
    assert_eq!(h.type_name(), "hash");
}

// --- Списки: lpush / lrange / llen ---

#[test]
fn lpush_pushes_to_front_and_reports_length() {
    let mut db = Database::new();
    assert_eq!(db.lpush("mylist", "a".to_string()), Ok(1));
    assert_eq!(db.lpush("mylist", "b".to_string()), Ok(2));
    assert_eq!(db.lpush("mylist", "c".to_string()), Ok(3));
    // LPUSH кладёт в начало, поэтому порядок обратный вставке.
    assert_eq!(
        db.lrange("mylist", 0, -1),
        vec!["c".to_string(), "b".to_string(), "a".to_string()]
    );
}

#[test]
fn llen_counts_list_elements() {
    let mut db = Database::new();
    db.lpush("l", "1".to_string()).unwrap();
    db.lpush("l", "2".to_string()).unwrap();
    assert_eq!(db.llen("l"), 2);
}

#[test]
fn llen_of_missing_key_is_zero() {
    let db = Database::new();
    assert_eq!(db.llen("ghost"), 0);
}

#[test]
fn lrange_handles_negative_indices() {
    let mut db = Database::new();
    // После трёх LPUSH: ["c", "b", "a"]
    db.lpush("l", "a".to_string()).unwrap();
    db.lpush("l", "b".to_string()).unwrap();
    db.lpush("l", "c".to_string()).unwrap();

    assert_eq!(db.lrange("l", 0, 0), vec!["c".to_string()]);
    assert_eq!(
        db.lrange("l", -2, -1),
        vec!["b".to_string(), "a".to_string()]
    );
    assert_eq!(
        db.lrange("l", 0, -1),
        vec!["c".to_string(), "b".to_string(), "a".to_string()]
    );
}

#[test]
fn lrange_empty_when_out_of_bounds_or_missing() {
    let mut db = Database::new();
    db.lpush("l", "only".to_string()).unwrap();
    let empty: Vec<String> = Vec::new();
    assert_eq!(db.lrange("missing", 0, -1), empty);
    // start за пределами длины → пусто
    assert_eq!(db.lrange("l", 5, 10), empty);
}

// --- Хеши: hset / hget / hkeys ---

#[test]
fn hset_new_field_returns_true_then_false_on_update() {
    let mut db = Database::new();
    assert_eq!(db.hset("user", "name", "alice".to_string()), Ok(true));
    // то же поле снова → перезапись, поле не новое
    assert_eq!(db.hset("user", "name", "bob".to_string()), Ok(false));
    assert_eq!(db.hget("user", "name"), Some("bob".to_string()));
}

#[test]
fn hget_missing_is_none() {
    let mut db = Database::new();
    db.hset("user", "name", "alice".to_string()).unwrap();
    assert_eq!(db.hget("user", "age"), None); // поля нет
    assert_eq!(db.hget("ghost", "name"), None); // ключа нет
}

#[test]
fn hkeys_are_sorted() {
    let mut db = Database::new();
    db.hset("h", "banana", "1".to_string()).unwrap();
    db.hset("h", "apple", "2".to_string()).unwrap();
    db.hset("h", "cherry", "3".to_string()).unwrap();
    assert_eq!(
        db.hkeys("h"),
        vec!["apple".to_string(), "banana".to_string(), "cherry".to_string()]
    );
}

// --- Неверный тип → Err (типизированность хранилища) ---

#[test]
fn lpush_on_string_is_wrongtype_error() {
    let mut db = Database::new();
    db.set("s", "hello".to_string());
    let res = db.lpush("s", "x".to_string());
    assert!(res.is_err(), "LPUSH по строке обязан вернуть Err");
    assert!(
        res.unwrap_err().starts_with("WRONGTYPE"),
        "сообщение об ошибке должно начинаться с WRONGTYPE"
    );
}

#[test]
fn hset_on_list_is_wrongtype_error() {
    let mut db = Database::new();
    db.lpush("l", "a".to_string()).unwrap();
    let res = db.hset("l", "field", "v".to_string());
    assert!(res.is_err(), "HSET по списку обязан вернуть Err");
    assert!(res.unwrap_err().starts_with("WRONGTYPE"));
}

#[test]
fn get_on_list_is_none_not_panic() {
    // GET по нестроковому значению — это не ошибка, а просто None.
    let mut db = Database::new();
    db.lpush("l", "a".to_string()).unwrap();
    assert_eq!(db.get("l"), None);
}

// --- del / keys / type_of ---

#[test]
fn del_removes_and_reports() {
    let mut db = Database::new();
    db.set("k", "v".to_string());
    assert!(db.del("k"));
    assert!(!db.del("k")); // второй раз — уже нет
    assert_eq!(db.get("k"), None);
}

#[test]
fn keys_are_sorted_and_cover_all_types() {
    let mut db = Database::new();
    db.set("str", "v".to_string());
    db.lpush("list", "a".to_string()).unwrap();
    db.hset("hash", "f", "v".to_string()).unwrap();
    assert_eq!(
        db.keys(),
        vec!["hash".to_string(), "list".to_string(), "str".to_string()]
    );
}

#[test]
fn type_of_reports_value_kind() {
    let mut db = Database::new();
    db.set("s", "v".to_string());
    db.lpush("l", "a".to_string()).unwrap();
    db.hset("h", "f", "v".to_string()).unwrap();

    assert_eq!(db.type_of("s"), Some("string"));
    assert_eq!(db.type_of("l"), Some("list"));
    assert_eq!(db.type_of("h"), Some("hash"));
    assert_eq!(db.type_of("missing"), None);
}

// =============================================================================
// Майлстоун 2 — текстовый протокол: execute(...)
// =============================================================================

#[test]
fn execute_set_get_del() {
    let mut db = Database::new();
    assert_eq!(execute(&mut db, "SET name alice"), "OK");
    assert_eq!(execute(&mut db, "GET name"), "alice");
    assert_eq!(execute(&mut db, "DEL name"), "(integer) 1");
    assert_eq!(execute(&mut db, "GET name"), "(nil)");
}

#[test]
fn execute_is_case_insensitive_for_command_name() {
    let mut db = Database::new();
    assert_eq!(execute(&mut db, "set k v"), "OK");
    // имя команды в любом регистре, но ключ/значение — как есть
    assert_eq!(execute(&mut db, "GeT k"), "v");
}

#[test]
fn execute_get_missing_is_nil() {
    let mut db = Database::new();
    assert_eq!(execute(&mut db, "GET nope"), "(nil)");
}

#[test]
fn execute_list_commands() {
    let mut db = Database::new();
    assert_eq!(execute(&mut db, "LPUSH l a"), "(integer) 1");
    assert_eq!(execute(&mut db, "LPUSH l b"), "(integer) 2");
    assert_eq!(execute(&mut db, "LLEN l"), "(integer) 2");
    assert_eq!(execute(&mut db, "LRANGE l 0 -1"), "b a");
}

#[test]
fn execute_hash_commands() {
    let mut db = Database::new();
    assert_eq!(execute(&mut db, "HSET u name alice"), "(integer) 1");
    assert_eq!(execute(&mut db, "HSET u name bob"), "(integer) 0");
    assert_eq!(execute(&mut db, "HGET u name"), "bob");
    assert_eq!(execute(&mut db, "HKEYS u"), "name");
}

#[test]
fn execute_type_and_keys() {
    let mut db = Database::new();
    execute(&mut db, "SET a 1");
    execute(&mut db, "SET b 2");
    assert_eq!(execute(&mut db, "TYPE a"), "string");
    assert_eq!(execute(&mut db, "TYPE missing"), "none");
    assert_eq!(execute(&mut db, "KEYS"), "a b");
}

#[test]
fn execute_wrongtype_is_reported_as_error() {
    let mut db = Database::new();
    execute(&mut db, "SET s hello");
    let resp = execute(&mut db, "LPUSH s x");
    assert!(
        resp.starts_with("(error) WRONGTYPE"),
        "ожидали '(error) WRONGTYPE ...', получили {resp:?}"
    );
}

#[test]
fn execute_unknown_command() {
    let mut db = Database::new();
    assert_eq!(execute(&mut db, "FLY high"), "(error) unknown command 'FLY'");
}

#[test]
fn execute_wrong_arity() {
    let mut db = Database::new();
    // SET без значения — недостаточно аргументов
    assert_eq!(
        execute(&mut db, "SET onlykey"),
        "(error) wrong number of arguments for 'SET'"
    );
}

#[test]
fn execute_lrange_non_integer_index() {
    let mut db = Database::new();
    execute(&mut db, "LPUSH l a");
    assert_eq!(
        execute(&mut db, "LRANGE l zero five"),
        "(error) value is not an integer"
    );
}

// =============================================================================
// Майлстоун 3 — персистентность: save / load (round-trip)
// =============================================================================

// Уникальное имя временного файла на каждый тест — детерминизм без коллизий при
// параллельном запуске. Никакой сети, только std::env::temp_dir().
static FILE_COUNTER: AtomicU64 = AtomicU64::new(0);

fn temp_path(tag: &str) -> String {
    let n = FILE_COUNTER.fetch_add(1, Ordering::Relaxed);
    let pid = std::process::id();
    let mut p = std::env::temp_dir();
    p.push(format!("minidb_{tag}_{pid}_{n}.db"));
    p.to_string_lossy().into_owned()
}

#[test]
fn save_then_load_roundtrips_all_types() {
    let path = temp_path("roundtrip");

    // Наполняем все три типа.
    let mut db = Database::new();
    db.set("greeting", "hello".to_string());
    db.lpush("queue", "first".to_string()).unwrap();
    db.lpush("queue", "second".to_string()).unwrap();
    db.hset("user", "name", "alice".to_string()).unwrap();
    db.hset("user", "city", "berlin".to_string()).unwrap();

    db.save(&path).expect("save должен записать файл во временный каталог");

    // Загружаем в свежую базу и сверяем.
    let mut restored = Database::new();
    restored.load(&path).expect("load должен прочитать ранее сохранённый файл");

    assert_eq!(restored.get("greeting"), Some("hello".to_string()));
    assert_eq!(
        restored.lrange("queue", 0, -1),
        vec!["second".to_string(), "first".to_string()]
    );
    assert_eq!(restored.hget("user", "name"), Some("alice".to_string()));
    assert_eq!(restored.hget("user", "city"), Some("berlin".to_string()));
    assert_eq!(
        restored.keys(),
        vec!["greeting".to_string(), "queue".to_string(), "user".to_string()]
    );

    let _ = std::fs::remove_file(&path);
}

#[test]
fn load_replaces_existing_contents() {
    let path = temp_path("replace");

    let mut src = Database::new();
    src.set("only", "this".to_string());
    src.save(&path).expect("save");

    // База с другими данными — после load они должны исчезнуть.
    let mut dst = Database::new();
    dst.set("stale", "gone".to_string());
    dst.load(&path).expect("load");

    assert_eq!(dst.get("only"), Some("this".to_string()));
    assert_eq!(dst.get("stale"), None);

    let _ = std::fs::remove_file(&path);
}

#[test]
fn execute_save_and_load_via_protocol() {
    let path = temp_path("protocol");

    let mut db = Database::new();
    execute(&mut db, "SET k v");
    assert_eq!(execute(&mut db, &format!("SAVE {path}")), "OK");

    let mut db2 = Database::new();
    assert_eq!(execute(&mut db2, &format!("LOAD {path}")), "OK");
    assert_eq!(execute(&mut db2, "GET k"), "v");

    let _ = std::fs::remove_file(&path);
}

// =============================================================================
// Майлстоун 4 — КОНКУРЕНТНОСТЬ: SharedDb (Arc<Mutex<Database>>)
// =============================================================================

#[test]
fn shared_db_basic_exec_works() {
    let db = SharedDb::new();
    assert_eq!(db.exec("SET k v"), "OK");
    assert_eq!(db.exec("GET k"), "v");
}

#[test]
fn shared_db_handle_points_to_same_store() {
    // handle() даёт второй дескриптор ТОГО ЖЕ хранилища, а не копию.
    let a = SharedDb::new();
    let b = a.handle();
    a.exec("SET shared yes");
    assert_eq!(b.exec("GET shared"), "yes");
}

#[test]
fn concurrent_lpush_counts_every_operation() {
    // Главный тест капстоуна: N потоков, каждый делает M операций LPUSH в ОДИН список.
    // Если Mutex действительно сериализует доступ, итоговая длина == N*M, и ни одна
    // вставка не потеряна. Числа фиксированы — тест детерминирован.
    const THREADS: usize = 8;
    const PER_THREAD: usize = 500;

    let db = SharedDb::new();

    let handles: Vec<_> = (0..THREADS)
        .map(|t| {
            let worker = db.handle(); // отдельный дескриптор для потока
            std::thread::spawn(move || {
                for i in 0..PER_THREAD {
                    // Значение уникально на (поток, итерацию) — но нам важна лишь длина.
                    let cmd = format!("LPUSH shared t{t}_i{i}");
                    let resp = worker.exec(&cmd);
                    assert!(
                        resp.starts_with("(integer) "),
                        "LPUSH должен вернуть длину, получили {resp:?}"
                    );
                }
            })
        })
        .collect();

    for h in handles {
        h.join().expect("поток не должен паниковать");
    }

    // Все вставки учтены — суммарная длина списка ровно THREADS*PER_THREAD.
    let expected = THREADS * PER_THREAD;
    assert_eq!(db.exec("LLEN shared"), format!("(integer) {expected}"));
}

#[test]
fn concurrent_independent_keys_do_not_interfere() {
    // Каждый поток пишет в СВОЙ ключ; в конце под каждым ключом ровно своя длина.
    const THREADS: usize = 4;
    const PER_THREAD: usize = 200;

    let db = SharedDb::new();
    let handles: Vec<_> = (0..THREADS)
        .map(|t| {
            let worker = db.handle();
            std::thread::spawn(move || {
                for i in 0..PER_THREAD {
                    worker.exec(&format!("LPUSH key{t} v{i}"));
                }
            })
        })
        .collect();

    for h in handles {
        h.join().unwrap();
    }

    for t in 0..THREADS {
        assert_eq!(
            db.exec(&format!("LLEN key{t}")),
            format!("(integer) {PER_THREAD}")
        );
    }
}

#[test]
fn shared_db_is_send_and_sync() {
    // Компиляция этого теста — уже проверка: SharedDb обязана быть Send + Sync, иначе
    // её нельзя было бы передать в thread::spawn в тестах выше. Эта функция-свидетель
    // не даст случайно потерять авто-трейты при рефакторинге обёртки.
    fn assert_send_sync<T: Send + Sync>() {}
    assert_send_sync::<SharedDb>();
}
