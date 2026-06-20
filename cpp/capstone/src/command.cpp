// command.cpp — РЕАЛИЗАЦИЯ разбора команд. Сейчас СТАБ: компилируется, но всегда отвечает "ERR",
// поэтому tests/test_command.cpp стартует КРАСНЫМ.
//
// Подсказки по реализации (детали — спрашивай меня):
//   - Разбей line на токены по пробелам (std::istringstream + оператор >> — модуль 08).
//   - Имя команды приведи к верхнему регистру, затем switch/if по нему.
//   - Аргументы-числа парси аккуратно: std::stol в try/catch (модуль 09) → при ошибке "ERR ...".
//   - Список/хеш-ответы склеивай через '\n'; пустые — "(empty)".
//   - НИКОГДА не давай исключению вылететь наружу: оберни тело в try/catch и верни "ERR ...".

#include "command.hpp"

#include <algorithm>
#include <cctype>
#include <sstream>
#include <string>
#include <vector>

namespace minidb {

namespace {

// Разбить строку на токены по пробельным символам (istringstream + operator>>).
std::vector<std::string> tokenize(const std::string& line) {
    std::vector<std::string> tokens;
    std::istringstream iss(line);
    std::string tok;
    while (iss >> tok) tokens.push_back(tok);
    return tokens;
}

// Имя команды к ВЕРХНЕМУ регистру: set и SET — одна команда.
std::string upper(std::string s) {
    std::transform(s.begin(), s.end(), s.begin(),
                   [](unsigned char c) { return static_cast<char>(std::toupper(c)); });
    return s;
}

// Склеить вектор строк через '\n'; пустой → "(empty)". Используется для LRANGE/HKEYS/KEYS.
std::string join_lines(const std::vector<std::string>& items) {
    if (items.empty()) return "(empty)";
    std::string out;
    for (std::size_t i = 0; i < items.size(); ++i) {
        if (i) out += '\n';
        out += items[i];
    }
    return out;
}

// Распарсить целое из токена строго (вся строка — число). Бросает при мусоре.
long parse_long(const std::string& s) {
    std::size_t pos = 0;
    long v = std::stol(s, &pos);  // бросит std::invalid_argument на нечисле
    if (pos != s.size()) throw std::invalid_argument("trailing chars");
    return v;
}

}  // namespace

// execute ВСЕГДА возвращает строку — любое исключение ловим и превращаем в "ERR ...".
// Это держит REPL и тесты простыми: слой команд не «протекает» исключениями наружу.
std::string execute(Database& db, const std::string& line) {
    try {
        auto tokens = tokenize(line);
        if (tokens.empty()) return "ERR empty command";

        const std::string cmd = upper(tokens[0]);
        const std::size_t argc = tokens.size() - 1;  // число аргументов без имени команды

        // Маленький помощник: число с префиксом ':' (Redis-стиль для целочисленных ответов).
        auto integer = [](long n) { return ":" + std::to_string(n); };

        // ------------------------------ strings ------------------------------
        if (cmd == "SET") {
            if (argc != 2) return "ERR wrong number of arguments";
            db.set(tokens[1], tokens[2]);
            return "OK";
        }
        if (cmd == "GET") {
            if (argc != 1) return "ERR wrong number of arguments";
            auto v = db.get(tokens[1]);
            return v ? *v : "(nil)";
        }

        // ------------------------------- lists -------------------------------
        if (cmd == "LPUSH") {
            if (argc != 2) return "ERR wrong number of arguments";
            return integer(static_cast<long>(db.lpush(tokens[1], tokens[2])));
        }
        if (cmd == "LRANGE") {
            if (argc != 3) return "ERR wrong number of arguments";
            long start = parse_long(tokens[2]);
            long stop = parse_long(tokens[3]);
            return join_lines(db.lrange(tokens[1], start, stop));
        }
        if (cmd == "LLEN") {
            if (argc != 1) return "ERR wrong number of arguments";
            return integer(static_cast<long>(db.llen(tokens[1])));
        }

        // ------------------------------- hashes ------------------------------
        if (cmd == "HSET") {
            if (argc != 3) return "ERR wrong number of arguments";
            return integer(db.hset(tokens[1], tokens[2], tokens[3]) ? 1 : 0);
        }
        if (cmd == "HGET") {
            if (argc != 2) return "ERR wrong number of arguments";
            auto v = db.hget(tokens[1], tokens[2]);
            return v ? *v : "(nil)";
        }
        if (cmd == "HKEYS") {
            if (argc != 1) return "ERR wrong number of arguments";
            return join_lines(db.hkeys(tokens[1]));
        }

        // ------------------------------ generic ------------------------------
        if (cmd == "DEL") {
            if (argc != 1) return "ERR wrong number of arguments";
            return integer(db.del(tokens[1]) ? 1 : 0);
        }
        if (cmd == "TYPE") {
            if (argc != 1) return "ERR wrong number of arguments";
            auto t = db.type(tokens[1]);
            if (!t) return "none";
            switch (*t) {
                case Type::String: return "string";
                case Type::List:   return "list";
                case Type::Hash:   return "hash";
            }
            return "none";
        }
        if (cmd == "KEYS") {
            if (argc != 0) return "ERR wrong number of arguments";
            return join_lines(db.keys());
        }
        if (cmd == "COUNT") {
            if (argc != 0) return "ERR wrong number of arguments";
            return integer(static_cast<long>(db.size()));
        }

        // ------------------------------- expiry ------------------------------
        if (cmd == "EXPIRE") {
            if (argc != 2) return "ERR wrong number of arguments";
            long secs = parse_long(tokens[2]);  // нечисло → catch → "ERR value is not an integer"
            return integer(db.expire(tokens[1], static_cast<std::int64_t>(secs)) ? 1 : 0);
        }
        if (cmd == "TTL") {
            if (argc != 1) return "ERR wrong number of arguments";
            return integer(static_cast<long>(db.ttl(tokens[1])));
        }
        if (cmd == "PERSIST") {
            if (argc != 1) return "ERR wrong number of arguments";
            return integer(db.persist(tokens[1]) ? 1 : 0);
        }

        // ---------------------------- persistence ----------------------------
        if (cmd == "SAVE") {
            if (argc != 1) return "ERR wrong number of arguments";
            return db.save(tokens[1]) ? "OK" : "ERR save failed";
        }
        if (cmd == "LOAD") {
            if (argc != 1) return "ERR wrong number of arguments";
            return db.load(tokens[1]) ? "OK" : "ERR load failed";
        }

        return "ERR unknown command";
    } catch (const std::invalid_argument&) {
        // Сюда падает parse_long на нечисловом аргументе.
        return "ERR value is not an integer";
    } catch (const std::out_of_range&) {
        // std::stol при переполнении long.
        return "ERR value is not an integer";
    } catch (const std::exception&) {
        // Любая иная ошибка (например, бросающий аксессор Value) не должна вылетать наружу.
        return "ERR internal error";
    }
}

}  // namespace minidb
