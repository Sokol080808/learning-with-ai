// Готовое приложение: читает весь текст со stdin и печатает 10 самых частых слов.
#include <iostream>
#include <sstream>
#include <string>
#include "word_freq.hpp"

int main() {
    std::ostringstream buffer;
    buffer << std::cin.rdbuf();          // прочитать весь stdin в строку
    const std::string text = buffer.str();

    auto freq = word_count(text);
    auto top = top_n(freq, 10);

    std::cout << "Топ слов:\n";
    for (const auto& [word, count] : top) {
        std::cout << count << "\t" << word << "\n";
    }
    return 0;
}
