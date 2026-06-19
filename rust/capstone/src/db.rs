//! Модуль `db` — однопоточное хранилище [`Database`].
//!
//! Это сердце капстоуна: `HashMap<String, Value>` плюс типизированные операции над ним.
//! Здесь ещё НЕТ ни потоков, ни `Mutex` — это обычная структура с `&self` / `&mut self`.
//! Потокобезопасность приедет отдельным слоем в модуле [`crate::shared`]; так и нужно
//! проектировать — сначала корректная логика, потом обёртка для конкурентности.
//!
//! Все операции уважают тип значения под ключом (см. [`crate::Value`]). Операции,
//! которые могут «не справиться» из-за неверного типа, возвращают `Result<_, String>`
//! (модуль 07): в `Err` — человекочитаемое сообщение об ошибке.

use crate::value::Value;
use std::collections::HashMap;

/// In-memory key-value хранилище.
///
/// Внутри — `HashMap<String, Value>`. Поле приватное: снаружи с базой работают ТОЛЬКО
/// через методы ниже. Так инварианты (например «тип значения соответствует операции»)
/// нельзя нарушить случайно.
#[derive(Debug, Default)]
pub struct Database {
    // Единственное состояние: отображение ключ → значение.
    map: HashMap<String, Value>,
}

impl Database {
    /// Создать пустое хранилище.
    pub fn new() -> Self {
        Database {
            map: HashMap::new(),
        }
    }

    // --- Строки: SET / GET ---------------------------------------------------

    /// `SET key value` — записать строку под ключ.
    ///
    /// Контракт:
    /// - кладёт `Value::Str(value)` под ключ `key`, **затирая** любое прежнее значение
    ///   (любого типа) — как `SET` в Redis;
    /// - `key: &str` мы копируем в `String` (хранилище владеет своими ключами);
    /// - `value: String` передаётся по значению — забираем владение и кладём внутрь.
    pub fn set(&mut self, key: &str, value: String) {
        self.map.insert(key.to_string(), Value::Str(value));
    }

    /// `GET key` — прочитать строку.
    ///
    /// Контракт:
    /// - если под `key` лежит `Value::Str(s)` — вернуть `Some(s.clone())`;
    /// - если ключа нет ИЛИ под ним значение другого типа (список/хеш) — вернуть `None`.
    ///
    /// Возвращаем `String` (копию), а не `&str`: иначе пришлось бы тащить наружу время
    /// жизни заимствования из хранилища, что неудобно для вызывающего.
    pub fn get(&self, key: &str) -> Option<String> {
        match self.map.get(key) {
            Some(Value::Str(s)) => Some(s.clone()),
            _ => None,
        }
    }

    // --- Списки: LPUSH / LRANGE / LLEN --------------------------------------

    /// `LPUSH key value` — добавить элемент в **начало** списка.
    ///
    /// Контракт:
    /// - если ключа нет — создать список из одного элемента;
    /// - если под ключом уже `Value::List` — вставить `value` в начало (индекс 0);
    /// - если под ключом значение ДРУГОГО типа (строка/хеш) — вернуть
    ///   `Err("WRONGTYPE: key holds a string, expected list".to_string())`
    ///   (или аналогичное сообщение, начинающееся с `WRONGTYPE`);
    /// - при успехе вернуть `Ok(новая_длина_списка)`.
    ///
    /// ПОЧЕМУ `Result`: «неверный тип» — восстановимая ошибка уровня логики, вызывающий
    /// сам решит, что с ней делать. Именно тут проявляется типизированность хранилища.
    /// Вставка в начало — `Vec::insert(0, value)`.
    pub fn lpush(&mut self, key: &str, value: String) -> Result<usize, String> {
        match self.map.get(key) {
            Some(Value::Str(_)) => {
                return Err(format!("WRONGTYPE key holds a string, expected list"));
            }
            Some(Value::Hash(_)) => {
                return Err(format!("WRONGTYPE key holds a hash, expected list"));
            }
            _ => {}
        }
        let list = self
            .map
            .entry(key.to_string())
            .or_insert_with(|| Value::List(Vec::new()));
        if let Value::List(v) = list {
            v.insert(0, value);
            Ok(v.len())
        } else {
            unreachable!()
        }
    }

    /// `LRANGE key start stop` — срез списка по индексам (включая `stop`).
    ///
    /// Контракт (повторяет семантику Redis):
    /// - индексы могут быть **отрицательными**: `-1` — последний элемент, `-2` —
    ///   предпоследний, и т.д. (отсчёт с конца);
    /// - после нормализации диапазон обрезается по границам `[0, len)`;
    /// - если ключа нет, значение не список, или диапазон пуст — вернуть пустой `Vec`;
    /// - иначе вернуть копии элементов с `start` по `stop` включительно, по порядку.
    ///
    /// Пример: список `["c", "b", "a"]` (после трёх LPUSH a, b, c), `LRANGE 0 -1`
    /// вернёт `["c", "b", "a"]`; `LRANGE 0 0` → `["c"]`; `LRANGE -2 -1` → `["b", "a"]`.
    pub fn lrange(&self, key: &str, start: i64, stop: i64) -> Vec<String> {
        let v = match self.map.get(key) {
            Some(Value::List(v)) => v,
            _ => return Vec::new(),
        };
        let len = v.len() as i64;
        // Normalize negative indices
        let start = if start < 0 { (len + start).max(0) } else { start };
        let stop = if stop < 0 { len + stop } else { stop };
        // Clip to valid bounds
        let start = start as usize;
        let stop = stop as usize;
        if start >= v.len() || stop < start {
            return Vec::new();
        }
        let stop = stop.min(v.len() - 1);
        v[start..=stop].to_vec()
    }

    /// `LLEN key` — длина списка.
    ///
    /// Контракт:
    /// - если под ключом `Value::List(v)` — вернуть `v.len()`;
    /// - если ключа нет или значение не список — вернуть `0`.
    pub fn llen(&self, key: &str) -> usize {
        match self.map.get(key) {
            Some(Value::List(v)) => v.len(),
            _ => 0,
        }
    }

    // --- Хеши: HSET / HGET / HKEYS ------------------------------------------

    /// `HSET key field value` — записать поле в хеш.
    ///
    /// Контракт:
    /// - если ключа нет — создать хеш с единственным полем;
    /// - если под ключом уже `Value::Hash` — записать `field -> value`;
    /// - если под ключом значение ДРУГОГО типа — вернуть `Err` с сообщением,
    ///   начинающимся на `WRONGTYPE` (как в `lpush`);
    /// - вернуть `Ok(true)`, если поле **новое** (раньше его в хеше не было),
    ///   и `Ok(false)`, если поле уже существовало и значение было перезаписано.
    ///   (Это поведение `HSET` в Redis: возвращается число добавленных полей.)
    pub fn hset(&mut self, key: &str, field: &str, value: String) -> Result<bool, String> {
        match self.map.get(key) {
            Some(Value::Str(_)) => {
                return Err(format!("WRONGTYPE key holds a string, expected hash"));
            }
            Some(Value::List(_)) => {
                return Err(format!("WRONGTYPE key holds a list, expected hash"));
            }
            _ => {}
        }
        let hash = self
            .map
            .entry(key.to_string())
            .or_insert_with(|| Value::Hash(HashMap::new()));
        if let Value::Hash(m) = hash {
            let is_new = !m.contains_key(field);
            m.insert(field.to_string(), value);
            Ok(is_new)
        } else {
            unreachable!()
        }
    }

    /// `HGET key field` — прочитать поле хеша.
    ///
    /// Контракт:
    /// - если под `key` хеш и в нём есть `field` — вернуть `Some(value.clone())`;
    /// - иначе (нет ключа / не хеш / нет поля) — `None`.
    pub fn hget(&self, key: &str, field: &str) -> Option<String> {
        match self.map.get(key) {
            Some(Value::Hash(m)) => m.get(field).cloned(),
            _ => None,
        }
    }

    /// `HKEYS key` — имена всех полей хеша, **отсортированные** по возрастанию.
    ///
    /// Контракт:
    /// - если под `key` хеш — вернуть его ключи как `Vec<String>`, отсортированные
    ///   лексикографически (порядок обхода `HashMap` не определён — сортировка делает
    ///   результат детерминированным, иначе тесты были бы флаки);
    /// - если ключа нет или значение не хеш — вернуть пустой `Vec`.
    pub fn hkeys(&self, key: &str) -> Vec<String> {
        match self.map.get(key) {
            Some(Value::Hash(m)) => {
                let mut keys: Vec<String> = m.keys().cloned().collect();
                keys.sort();
                keys
            }
            _ => Vec::new(),
        }
    }

    // --- Общие операции: DEL / KEYS / TYPE ----------------------------------

    /// `DEL key` — удалить ключ.
    ///
    /// Контракт:
    /// - вернуть `true`, если ключ существовал и был удалён;
    /// - вернуть `false`, если ключа не было.
    pub fn del(&mut self, key: &str) -> bool {
        self.map.remove(key).is_some()
    }

    /// `KEYS` — все ключи хранилища, **отсортированные** по возрастанию.
    ///
    /// Сортировка — ради детерминизма (см. `hkeys`).
    pub fn keys(&self) -> Vec<String> {
        let mut keys: Vec<String> = self.map.keys().cloned().collect();
        keys.sort();
        keys
    }

    /// `TYPE key` — тип значения под ключом.
    ///
    /// Контракт:
    /// - вернуть `Some("string"|"list"|"hash")`, если ключ есть (см.
    ///   [`crate::Value::type_name`]);
    /// - вернуть `None`, если ключа нет.
    pub fn type_of(&self, key: &str) -> Option<&'static str> {
        self.map.get(key).map(|v| v.type_name())
    }

    // --- Персистентность: SAVE / LOAD ---------------------------------------

    /// `SAVE path` — сохранить всё хранилище в файл по пути `path`.
    ///
    /// Формат сериализации ты выбираешь сам (главное — чтобы `load` читал то, что пишет
    /// `save`: round-trip обязан восстановить ровно те же данные). Рекомендация —
    /// простой строчный текстовый формат, по строке на запись, например:
    ///
    /// ```text
    /// S<TAB>key<TAB>value
    /// L<TAB>key<TAB>elem0<TAB>elem1<TAB>...
    /// H<TAB>key<TAB>field0<TAB>val0<TAB>field1<TAB>val1<TAB>...
    /// ```
    ///
    /// (`<TAB>` — символ табуляции `'\t'`. Свой формат тоже годится.)
    ///
    /// Возвращает `std::io::Result<()>`: операция работает с файловой системой и может
    /// не справиться (нет прав, нет каталога). Внутри пригодится оператор `?` (модуль 07)
    /// для проброса I/O-ошибок, и `std::fs::write` / `std::fs::File` + запись строк.
    pub fn save(&self, path: &str) -> std::io::Result<()> {
        use std::io::Write;
        let mut file = std::fs::File::create(path)?;
        for (key, value) in &self.map {
            match value {
                Value::Str(s) => {
                    writeln!(file, "S\t{key}\t{s}")?;
                }
                Value::List(v) => {
                    let elems = v.join("\t");
                    writeln!(file, "L\t{key}\t{elems}")?;
                }
                Value::Hash(m) => {
                    let mut parts = Vec::new();
                    for (field, val) in m {
                        parts.push(field.clone());
                        parts.push(val.clone());
                    }
                    let encoded = parts.join("\t");
                    writeln!(file, "H\t{key}\t{encoded}")?;
                }
            }
        }
        Ok(())
    }

    /// `LOAD path` — загрузить хранилище из файла (заменяя текущее содержимое).
    ///
    /// Контракт:
    /// - прочитать файл по пути `path`, разобрать в `HashMap<String, Value>` тем же
    ///   форматом, что пишет [`Database::save`], и **заменить** текущее содержимое;
    /// - вернуть `Ok(())` при успехе;
    /// - I/O-ошибки (файла нет и т.п.) пробрасывать как `Err` (тип `std::io::Result`).
    ///
    /// Главное требование: `save` потом `load` восстанавливает ИСХОДНОЕ состояние
    /// (round-trip). Это проверяет приёмочный тест на персистентность.
    pub fn load(&mut self, path: &str) -> std::io::Result<()> {
        use std::io::{BufRead, BufReader};
        let file = std::fs::File::open(path)?;
        let reader = BufReader::new(file);
        let mut new_map: HashMap<String, Value> = HashMap::new();
        for line in reader.lines() {
            let line = line?;
            if line.is_empty() {
                continue;
            }
            let parts: Vec<&str> = line.splitn(3, '\t').collect();
            if parts.len() < 2 {
                continue;
            }
            let tag = parts[0];
            let key = parts[1].to_string();
            let rest = if parts.len() == 3 { parts[2] } else { "" };
            match tag {
                "S" => {
                    new_map.insert(key, Value::Str(rest.to_string()));
                }
                "L" => {
                    let elems: Vec<String> = if rest.is_empty() {
                        Vec::new()
                    } else {
                        rest.split('\t').map(|s| s.to_string()).collect()
                    };
                    new_map.insert(key, Value::List(elems));
                }
                "H" => {
                    let tokens: Vec<&str> = rest.split('\t').collect();
                    let mut m = HashMap::new();
                    let mut i = 0;
                    while i + 1 < tokens.len() {
                        m.insert(tokens[i].to_string(), tokens[i + 1].to_string());
                        i += 2;
                    }
                    new_map.insert(key, Value::Hash(m));
                }
                _ => {}
            }
        }
        self.map = new_map;
        Ok(())
    }
}
