// value.hpp — типизированное значение MiniDB.
//
// Это «контракт» (интерфейс) одного из трёх кирпичей капстоуна. РЕАЛИЗАЦИИ здесь нет —
// тела живут в src/value.cpp (сейчас там стабы). Твоя задача в этом файле — НИЧЕГО не менять
// в сигнатурах: тесты (tests/test_value.cpp) зовут именно эти методы с именно такими типами.
//
// Идея. Значение в Redis-подобной БД может быть РАЗНЫМ по форме: строкой, списком строк или
// хешем (map поле→строка). Раньше (kvstore) у нас были только строки. Теперь нужен один тип
// Value, который умеет хранить ЛЮБУЮ из трёх форм и безопасно сообщать, что внутри. Это ровно
// тот случай, под который придуман std::variant (модули 07/09) — «один из нескольких типов».

#pragma once

#include <cstdint>
#include <map>
#include <string>
#include <variant>
#include <vector>

namespace minidb {

// Какая из трёх форм лежит внутри Value. enum class — строго типизированный (модуль 04),
// не путается с int и не «протекает» именами в окружающее пространство.
enum class Type { String, List, Hash };

// Псевдонимы для читаемости. Внутреннее хранилище — std::variant из ровно этих трёх типов.
// ПОРЯДОК альтернатив в variant ДОЛЖЕН совпадать с порядком в enum Type (String=0, List=1,
// Hash=2) — это упростит реализацию type().
using ListData = std::vector<std::string>;
using HashData = std::map<std::string, std::string>;
using Storage  = std::variant<std::string, ListData, HashData>;

class Value {
public:
    // --- Фабрики (named constructors). Создавать Value только через них. ---
    // Почему фабрики, а не публичные конструкторы: имя make_list() самодокументирует, ЧТО
    // создаётся, и не даёт случайно собрать Value из «голого» variant в неверном состоянии.
    static Value make_string(std::string s);
    static Value make_list();   // пустой список
    static Value make_hash();   // пустой хеш

    // Какая форма внутри. Дёшево и без копий.
    Type type() const;

    // --- Типизированные аксессоры. ---
    // Возвращают ССЫЛКУ на внутренние данные нужного типа. Если форма не та (например, зовёшь
    // as_list() у строки) — бросают std::runtime_error (модуль 09). Это «защита от дурака»:
    // лучше явная ошибка, чем тихо неверные данные.
    //
    // Пара const/неconst (модуль 04): неconst-версия даёт менять данные на месте
    // (например, lpush добавляет элемент в возвращённый вектор), const-версия — только читать.
    std::string&       as_string();
    const std::string& as_string() const;

    ListData&       as_list();
    const ListData& as_list() const;

    HashData&       as_hash();
    const HashData& as_hash() const;

private:
    // Приватный конструктор: снаружи Value собирается ТОЛЬКО фабриками выше.
    explicit Value(Storage data);

    Storage data_;
};

}  // namespace minidb
