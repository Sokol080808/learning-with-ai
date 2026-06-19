// main.cpp — РАБОЧИЙ REPL для MiniDB (это НЕ стаб: читать тут нечего реализовывать).
//
// Цикл: печатаем приглашение, читаем строку, отдаём её execute(db, line), печатаем ответ.
// Сама логика команд живёт в src/command.cpp — пока там стаб, поэтому REPL будет исправно
// отвечать "ERR not implemented" на всё, КРОМЕ выхода. Как только ты реализуешь Database и
// execute, этот же REPL «оживёт» без единой правки здесь. В этом и смысл слоёв.
//
// Пример сессии (после того как реализуешь логику):
//   minidb> SET name Ada
//   OK
//   minidb> GET name
//   Ada
//   minidb> LPUSH todo wake
//   :1
//   minidb> TTL name
//   :-1
//   minidb> QUIT
//   bye

#include <iostream>
#include <string>

#include "command.hpp"
#include "database.hpp"

int main() {
    minidb::Database db;  // реальные системные часы по умолчанию
    std::string line;

    std::cout << "MiniDB REPL. Команды: SET/GET/DEL/TYPE/KEYS/COUNT, LPUSH/LRANGE/LLEN,\n"
                 "HSET/HGET/HKEYS, EXPIRE/TTL/PERSIST, SAVE/LOAD. Выход: QUIT.\n";

    while (true) {
        std::cout << "minidb> " << std::flush;
        if (!std::getline(std::cin, line)) {
            // Ctrl-D / конец ввода — выходим аккуратно.
            std::cout << "\nbye\n";
            break;
        }

        // Локальная обработка выхода — не нагружаем ею слой команд.
        if (line == "QUIT" || line == "quit" || line == "EXIT" || line == "exit") {
            std::cout << "bye\n";
            break;
        }
        if (line.empty()) {
            continue;
        }

        std::cout << minidb::execute(db, line) << '\n';
    }

    return 0;
}
