// REPL для kvstore. Готов к работе: использует твои KvStore и execute().
// Команда QUIT или конец ввода (Ctrl-D) завершает программу.
#include <iostream>
#include <string>
#include "kvstore.hpp"
#include "command.hpp"

int main() {
    KvStore store;
    std::string line;

    std::cout << "kvstore. Команды: SET GET DEL COUNT KEYS SAVE LOAD QUIT\n";
    std::cout << "> " << std::flush;
    while (std::getline(std::cin, line)) {
        if (line == "QUIT" || line == "quit") break;
        if (!line.empty()) {
            std::cout << execute(store, line) << "\n";
        }
        std::cout << "> " << std::flush;
    }
    return 0;
}
