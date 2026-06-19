// command.hpp — текстовый интерфейс MiniDB: одна строка команды → одна строка ответа.
//
// РЕАЛИЗАЦИЯ — в src/command.cpp (сейчас стаб). Сигнатуру МЕНЯТЬ НЕЛЬЗЯ:
// tests/test_command.cpp и app/main.cpp (REPL) зовут именно execute(db, line).
//
// Идея. Database — это типизированный C++ API. Чтобы с базой можно было «разговаривать» текстом
// (как redis-cli), нужен слой разбора: распарсить строку "SET name Ada" в вызов db.set("name","Ada")
// и обратно превратить результат в Redis-подобный ответ. Здесь работают строки и парсинг (модуль 08),
// optional/исключения (модуль 09).

#pragma once

#include <string>

#include "database.hpp"

namespace minidb {

// Разобрать ОДНУ строку команды, выполнить её над db и вернуть строку-ответ (БЕЗ завершающего '\n').
//
// Поддерживаемые команды (регистр имени — нечувствителен: set == SET) и их ответы:
//
//   strings:  SET key value        -> "OK"
//             GET key              -> значение | "(nil)"
//   lists:    LPUSH key value      -> ":<новая длина>"           (число с префиксом ':')
//             LRANGE key i j       -> элементы по одному в строке (через '\n'); пусто -> "(empty)"
//             LLEN key             -> ":<длина>"
//   hashes:   HSET key field value -> ":1" если поле новое, ":0" если перезаписано
//             HGET key field       -> значение | "(nil)"
//             HKEYS key            -> поля по одному в строке (через '\n'); пусто -> "(empty)"
//   generic:  DEL key              -> ":1" если удалён, ":0" если ключа не было
//             TYPE key             -> "string" | "list" | "hash" | "none"
//             KEYS                 -> все живые ключи через '\n'; пусто -> "(empty)"
//             COUNT                -> ":<число живых ключей>"
//   expiry:   EXPIRE key seconds   -> ":1" если ключ есть, ":0" если нет
//             TTL key              -> ":<секунды | -1 | -2>"
//             PERSIST key          -> ":1" если TTL снят, ":0" иначе
//   persist:  SAVE path            -> "OK" | "ERR save failed"
//             LOAD path            -> "OK" | "ERR load failed"
//
// Ошибки разбора (неизвестная команда, не хватает аргументов, аргумент не число и т.п.) —
// строка вида "ERR <короткое пояснение>". Никогда не кидать исключение наружу: execute
// ВСЕГДА возвращает строку (даже на мусор). Это упрощает REPL и тесты.
std::string execute(Database& db, const std::string& line);

}  // namespace minidb
