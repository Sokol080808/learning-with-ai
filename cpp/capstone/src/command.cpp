#include "command.hpp"
// #include <sstream>

std::string execute(KvStore& store, const std::string& line) {
    // TODO (майлстоун 2 и 3): разобрать команду и выполнить её.
    //
    // Идея разбора: возьми первый токен — это команда (SET/GET/DEL/COUNT/KEYS/SAVE/LOAD).
    // Дальше — аргументы. Для SET значение — это ВЕСЬ остаток строки после ключа
    // (может содержать пробелы), поэтому не дроби его на токены.
    (void)store; (void)line;
    return "ERR unknown command";
}
