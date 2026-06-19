//! Модуль `command` — текстовый протокол: разбор строки и исполнение команды.
//!
//! [`execute`] — это «мозг» REPL и сетевого слоя: на входе одна строка вроде
//! `"SET name alice"`, на выходе строковый ответ в Redis-подобном стиле (`"OK"`,
//! `"(nil)"`, `"(integer) 3"`, …). Здесь нет состояния — только разбор и вызов методов
//! [`crate::Database`]. Тот же `execute` используют и [`crate::SharedDb::exec`], и
//! бинарник `main.rs`.

use crate::db::Database;

/// Разобрать и исполнить одну команду над хранилищем, вернув текстовый ответ.
///
/// Команда — это слова, разделённые пробелами. Первое слово — имя команды
/// (регистронезависимо: `SET`, `set`, `Set` — одно и то же). Остальные слова —
/// аргументы. Значение для `SET` / `LPUSH` / `HSET` — это **последний** аргумент
/// (для простоты считаем, что значения без пробелов; обработку кавычек можешь добавить
/// как стретч-цель).
///
/// Поддерживаемые команды и их ответы (эталон — приёмочные тесты):
///
/// | Команда                 | Успех                                  | Особые случаи                          |
/// |-------------------------|----------------------------------------|----------------------------------------|
/// | `SET key value`         | `OK`                                   |                                        |
/// | `GET key`               | значение                               | нет ключа/не строка → `(nil)`          |
/// | `DEL key`               | `(integer) 1`                          | ключа не было → `(integer) 0`          |
/// | `TYPE key`              | `string` \| `list` \| `hash`           | нет ключа → `none`                     |
/// | `KEYS`                  | ключи через пробел (сорт.)             | пусто → `(empty)`                      |
/// | `LPUSH key value`       | `(integer) <новая длина>`              | неверный тип → `(error) WRONGTYPE ...` |
/// | `LRANGE key start stop` | элементы через пробел                  | пусто → `(empty)`                      |
/// | `LLEN key`              | `(integer) <длина>`                    |                                        |
/// | `HSET key field value`  | `(integer) 1` (новое) / `(integer) 0`  | неверный тип → `(error) WRONGTYPE ...` |
/// | `HGET key field`        | значение                               | нет → `(nil)`                          |
/// | `HKEYS key`             | поля через пробел (сорт.)              | пусто → `(empty)`                      |
/// | `SAVE path`             | `OK`                                   | ошибка I/O → `(error) <текст>`         |
/// | `LOAD path`             | `OK`                                   | ошибка I/O → `(error) <текст>`         |
///
/// Общие правила ответов:
/// - неизвестная команда → `(error) unknown command '<имя>'`;
/// - нехватка аргументов → `(error) wrong number of arguments for '<имя>'`;
/// - в `LRANGE` нечисловые `start`/`stop` → `(error) value is not an integer`;
/// - для `Err(msg)` от методов базы (неверный тип) ответ — `(error) <msg>`.
///
/// ПОЧЕМУ всё это возвращается строкой, а не печатается тут же: так `execute` остаётся
/// чистой функцией (вход → выход), её легко тестировать и переиспользовать в потоках.
/// Реализуй через разбор `line.split_whitespace()` и `match` по имени команды (модуль 06),
/// прокидывая ошибки методов базы (модуль 07) в формат `(error) ...`.
pub fn execute(db: &mut Database, line: &str) -> String {
    let args: Vec<&str> = line.split_whitespace().collect();
    if args.is_empty() {
        return String::new();
    }
    let cmd = args[0].to_uppercase();
    match cmd.as_str() {
        "SET" => {
            if args.len() < 3 {
                return format!("(error) wrong number of arguments for 'SET'");
            }
            db.set(args[1], args[2].to_string());
            "OK".to_string()
        }
        "GET" => {
            if args.len() < 2 {
                return format!("(error) wrong number of arguments for 'GET'");
            }
            match db.get(args[1]) {
                Some(s) => s,
                None => "(nil)".to_string(),
            }
        }
        "DEL" => {
            if args.len() < 2 {
                return format!("(error) wrong number of arguments for 'DEL'");
            }
            let deleted = db.del(args[1]);
            format!("(integer) {}", if deleted { 1 } else { 0 })
        }
        "TYPE" => {
            if args.len() < 2 {
                return format!("(error) wrong number of arguments for 'TYPE'");
            }
            match db.type_of(args[1]) {
                Some(t) => t.to_string(),
                None => "none".to_string(),
            }
        }
        "KEYS" => {
            let keys = db.keys();
            if keys.is_empty() {
                "(empty)".to_string()
            } else {
                keys.join(" ")
            }
        }
        "LPUSH" => {
            if args.len() < 3 {
                return format!("(error) wrong number of arguments for 'LPUSH'");
            }
            match db.lpush(args[1], args[2].to_string()) {
                Ok(len) => format!("(integer) {len}"),
                Err(msg) => format!("(error) {msg}"),
            }
        }
        "LRANGE" => {
            if args.len() < 4 {
                return format!("(error) wrong number of arguments for 'LRANGE'");
            }
            let start = match args[2].parse::<i64>() {
                Ok(n) => n,
                Err(_) => return "(error) value is not an integer".to_string(),
            };
            let stop = match args[3].parse::<i64>() {
                Ok(n) => n,
                Err(_) => return "(error) value is not an integer".to_string(),
            };
            let elems = db.lrange(args[1], start, stop);
            if elems.is_empty() {
                "(empty)".to_string()
            } else {
                elems.join(" ")
            }
        }
        "LLEN" => {
            if args.len() < 2 {
                return format!("(error) wrong number of arguments for 'LLEN'");
            }
            format!("(integer) {}", db.llen(args[1]))
        }
        "HSET" => {
            if args.len() < 4 {
                return format!("(error) wrong number of arguments for 'HSET'");
            }
            match db.hset(args[1], args[2], args[3].to_string()) {
                Ok(is_new) => format!("(integer) {}", if is_new { 1 } else { 0 }),
                Err(msg) => format!("(error) {msg}"),
            }
        }
        "HGET" => {
            if args.len() < 3 {
                return format!("(error) wrong number of arguments for 'HGET'");
            }
            match db.hget(args[1], args[2]) {
                Some(v) => v,
                None => "(nil)".to_string(),
            }
        }
        "HKEYS" => {
            if args.len() < 2 {
                return format!("(error) wrong number of arguments for 'HKEYS'");
            }
            let keys = db.hkeys(args[1]);
            if keys.is_empty() {
                "(empty)".to_string()
            } else {
                keys.join(" ")
            }
        }
        "SAVE" => {
            if args.len() < 2 {
                return format!("(error) wrong number of arguments for 'SAVE'");
            }
            match db.save(args[1]) {
                Ok(()) => "OK".to_string(),
                Err(e) => format!("(error) {e}"),
            }
        }
        "LOAD" => {
            if args.len() < 2 {
                return format!("(error) wrong number of arguments for 'LOAD'");
            }
            match db.load(args[1]) {
                Ok(()) => "OK".to_string(),
                Err(e) => format!("(error) {e}"),
            }
        }
        _ => {
            format!("(error) unknown command '{}'", cmd)
        }
    }
}
