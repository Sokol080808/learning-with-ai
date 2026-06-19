// Эти тесты трогать не нужно — это эталон поведения (приёмочные тесты капстоуна).
// Они описывают, КАК должно вести себя готовое хранилище и язык команд. Сейчас они
// падают (методы возвращают todo!()), но компилируются. Твоя цель — сделать их зелёными.
//
// Запуск:  cargo test -p kvstore     (или собрать весь воркспейс: ./rust/run.sh)
//
// Тесты трогают ТОЛЬКО публичный API: методы KvStore и функцию execute.

use kvstore::*;

use std::path::PathBuf;
use std::process;
use std::sync::atomic::{AtomicU64, Ordering};

// --- Утилита: уникальный путь во временной папке -----------------------------
//
// Персистентность проверяем на настоящем файле, но кладём его в системную temp-папку
// (std::env::temp_dir) под уникальным именем, чтобы тесты не мешали друг другу и не
// зависели от текущего каталога. Счётчик + pid делают имя уникальным даже при
// параллельном запуске тестов.
static COUNTER: AtomicU64 = AtomicU64::new(0);

fn temp_path(tag: &str) -> PathBuf {
    let n = COUNTER.fetch_add(1, Ordering::Relaxed);
    let mut p = std::env::temp_dir();
    p.push(format!("kvstore_acc_{}_{}_{}.db", process::id(), tag, n));
    p
}

// =============================================================================
// МАЙЛСТОУН 1 — ядро KvStore (set / get / remove / len / keys)
// =============================================================================

#[test]
fn new_store_is_empty() {
    let store = KvStore::new();
    assert_eq!(store.len(), 0);
    assert!(store.is_empty());
    assert_eq!(store.get("nope"), None);
    assert_eq!(store.keys(), Vec::<String>::new());
}

#[test]
fn set_then_get_returns_value() {
    let mut store = KvStore::new();
    store.set("name".to_string(), "Alice".to_string());
    assert_eq!(store.get("name"), Some("Alice".to_string()));
    assert_eq!(store.len(), 1);
    assert!(!store.is_empty());
}

#[test]
fn set_overwrites_existing_key() {
    let mut store = KvStore::new();
    store.set("k".to_string(), "v1".to_string());
    store.set("k".to_string(), "v2".to_string());
    // Перезапись, а не вторая пара: значение новое, длина прежняя.
    assert_eq!(store.get("k"), Some("v2".to_string()));
    assert_eq!(store.len(), 1);
}

#[test]
fn get_missing_key_is_none() {
    let mut store = KvStore::new();
    store.set("a".to_string(), "1".to_string());
    assert_eq!(store.get("b"), None);
}

#[test]
fn remove_existing_returns_true_and_shrinks() {
    let mut store = KvStore::new();
    store.set("a".to_string(), "1".to_string());
    store.set("b".to_string(), "2".to_string());
    assert!(store.remove("a"));
    assert_eq!(store.get("a"), None);
    assert_eq!(store.len(), 1);
}

#[test]
fn remove_missing_returns_false() {
    let mut store = KvStore::new();
    store.set("a".to_string(), "1".to_string());
    assert!(!store.remove("zzz"));
    assert_eq!(store.len(), 1);
}

#[test]
fn keys_are_sorted_ascending() {
    let mut store = KvStore::new();
    // Вставляем в заведомо НЕ отсортированном порядке.
    store.set("banana".to_string(), "1".to_string());
    store.set("apple".to_string(), "2".to_string());
    store.set("cherry".to_string(), "3".to_string());
    // keys() обязан вернуть лексикографически по возрастанию, независимо от порядка вставки.
    assert_eq!(
        store.keys(),
        vec![
            "apple".to_string(),
            "banana".to_string(),
            "cherry".to_string()
        ]
    );
}

#[test]
fn value_may_contain_spaces() {
    // На уровне API значение — обычная строка; пробелы внутри не проблема.
    let mut store = KvStore::new();
    store.set("greeting".to_string(), "hello big world".to_string());
    assert_eq!(store.get("greeting"), Some("hello big world".to_string()));
}

// =============================================================================
// МАЙЛСТОУН 2 — разбор команд (execute)
// =============================================================================

#[test]
fn execute_set_returns_ok_and_stores() {
    let mut store = KvStore::new();
    assert_eq!(execute(&mut store, "SET name Alice"), "OK");
    assert_eq!(store.get("name"), Some("Alice".to_string()));
}

#[test]
fn execute_set_value_keeps_internal_spaces() {
    // Значение — это ВЕСЬ остаток строки после ключа, включая пробелы между словами.
    let mut store = KvStore::new();
    assert_eq!(execute(&mut store, "SET msg hello big world"), "OK");
    assert_eq!(store.get("msg"), Some("hello big world".to_string()));
    // И через GET тоже:
    assert_eq!(execute(&mut store, "GET msg"), "hello big world");
}

#[test]
fn execute_get_existing_and_missing() {
    let mut store = KvStore::new();
    execute(&mut store, "SET k v");
    assert_eq!(execute(&mut store, "GET k"), "v");
    // Отсутствующий ключ → ровно "(nil)".
    assert_eq!(execute(&mut store, "GET nope"), "(nil)");
}

#[test]
fn execute_del_existing_returns_ok_missing_returns_nil() {
    let mut store = KvStore::new();
    execute(&mut store, "SET k v");
    assert_eq!(execute(&mut store, "DEL k"), "OK");
    // Повторное удаление того же ключа — его уже нет.
    assert_eq!(execute(&mut store, "DEL k"), "(nil)");
    assert_eq!(store.len(), 0);
}

#[test]
fn execute_count_reports_size_as_number() {
    let mut store = KvStore::new();
    assert_eq!(execute(&mut store, "COUNT"), "0");
    execute(&mut store, "SET a 1");
    execute(&mut store, "SET b 2");
    assert_eq!(execute(&mut store, "COUNT"), "2");
}

#[test]
fn execute_keys_space_separated_sorted() {
    let mut store = KvStore::new();
    execute(&mut store, "SET banana 1");
    execute(&mut store, "SET apple 2");
    execute(&mut store, "SET cherry 3");
    // Ключи через один пробел, в отсортированном порядке.
    assert_eq!(execute(&mut store, "KEYS"), "apple banana cherry");
}

#[test]
fn execute_keys_empty_store_is_empty_string() {
    let mut store = KvStore::new();
    assert_eq!(execute(&mut store, "KEYS"), "");
}

#[test]
fn execute_unknown_command_is_err() {
    let mut store = KvStore::new();
    let resp = execute(&mut store, "FOOBAR x y");
    assert!(
        resp.starts_with("ERR "),
        "ответ на неизвестную команду должен начинаться с 'ERR ', получено: {resp:?}"
    );
}

#[test]
fn execute_empty_line_is_err() {
    let mut store = KvStore::new();
    // Пустая (или из одних пробелов) строка, дошедшая до execute, — это ошибка.
    let resp = execute(&mut store, "   ");
    assert!(
        resp.starts_with("ERR "),
        "ответ на пустую строку должен начинаться с 'ERR ', получено: {resp:?}"
    );
}

#[test]
fn execute_get_without_key_is_err() {
    let mut store = KvStore::new();
    let resp = execute(&mut store, "GET");
    assert!(
        resp.starts_with("ERR "),
        "GET без ключа — ошибка, получено: {resp:?}"
    );
}

#[test]
fn execute_set_without_value_is_err() {
    let mut store = KvStore::new();
    // SET с ключом, но БЕЗ значения — недостаточно аргументов.
    let resp = execute(&mut store, "SET onlykey");
    assert!(
        resp.starts_with("ERR "),
        "SET без значения — ошибка, получено: {resp:?}"
    );
}

// =============================================================================
// МАЙЛСТОУН 3 — персистентность (save + load через временный файл)
// =============================================================================

#[test]
fn save_then_load_roundtrips_all_pairs() {
    let path = temp_path("roundtrip");
    let path_str = path.to_str().expect("путь не UTF-8");

    let mut a = KvStore::new();
    a.set("name".to_string(), "Alice".to_string());
    a.set("city".to_string(), "Berlin".to_string());
    a.set("note".to_string(), "has spaces inside".to_string());
    a.save(path_str).expect("save должен пройти");

    // Загружаем в ДРУГОЕ, изначально пустое хранилище — и получаем те же пары.
    let mut b = KvStore::new();
    b.load(path_str).expect("load должен пройти");

    assert_eq!(b.len(), 3);
    assert_eq!(b.get("name"), Some("Alice".to_string()));
    assert_eq!(b.get("city"), Some("Berlin".to_string()));
    // Пробелы внутри значения переживают круг save→load.
    assert_eq!(b.get("note"), Some("has spaces inside".to_string()));
    assert_eq!(
        b.keys(),
        vec!["city".to_string(), "name".to_string(), "note".to_string()]
    );

    let _ = std::fs::remove_file(&path);
}

#[test]
fn load_replaces_existing_contents() {
    let path = temp_path("replace");
    let path_str = path.to_str().expect("путь не UTF-8");

    // Файл содержит только пару x=1.
    let mut src = KvStore::new();
    src.set("x".to_string(), "1".to_string());
    src.save(path_str).expect("save должен пройти");

    // Целевое хранилище заранее набито другими ключами — они должны исчезнуть после load.
    let mut dst = KvStore::new();
    dst.set("old1".to_string(), "a".to_string());
    dst.set("old2".to_string(), "b".to_string());
    dst.load(path_str).expect("load должен пройти");

    assert_eq!(dst.len(), 1);
    assert_eq!(dst.get("x"), Some("1".to_string()));
    assert_eq!(dst.get("old1"), None);
    assert_eq!(dst.get("old2"), None);

    let _ = std::fs::remove_file(&path);
}

#[test]
fn save_load_empty_store() {
    let path = temp_path("empty");
    let path_str = path.to_str().expect("путь не UTF-8");

    let empty = KvStore::new();
    empty.save(path_str).expect("save пустого должен пройти");

    let mut loaded = KvStore::new();
    loaded.set("garbage".to_string(), "x".to_string());
    loaded.load(path_str).expect("load должен пройти");
    assert_eq!(loaded.len(), 0);
    assert!(loaded.is_empty());

    let _ = std::fs::remove_file(&path);
}

#[test]
fn load_missing_file_is_err() {
    // Файла заведомо нет → load обязан вернуть Err (io::Error), а НЕ паниковать.
    let path = temp_path("does_not_exist");
    let path_str = path.to_str().expect("путь не UTF-8");
    let mut store = KvStore::new();
    let res = store.load(path_str);
    assert!(res.is_err(), "load несуществующего файла должен вернуть Err");
}

#[test]
fn execute_save_and_load_via_commands() {
    // Тот же круг save→load, но через язык команд: SAVE/LOAD должны отвечать "OK".
    let path = temp_path("cmd");
    let path_str = path.to_str().expect("путь не UTF-8");

    let mut a = KvStore::new();
    execute(&mut a, "SET k hello there");
    assert_eq!(execute(&mut a, &format!("SAVE {path_str}")), "OK");

    let mut b = KvStore::new();
    assert_eq!(execute(&mut b, &format!("LOAD {path_str}")), "OK");
    assert_eq!(execute(&mut b, "GET k"), "hello there");

    let _ = std::fs::remove_file(&path);
}
