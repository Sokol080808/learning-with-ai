#include "geometry/textstats.hpp"
// #include <cctype>   // std::isspace, std::toupper — пригодятся

namespace textstats {

int word_count(std::string_view text) {
    // TODO: посчитать слова — группы непробельных символов.
    (void)text;
    return -1;  // заведомо неверное значение, чтобы тест был красным
}

int longest_word(std::string_view text) {
    // TODO: пройти по словам, вернуть максимальную длину.
    (void)text;
    return -1;
}

std::string to_upper(std::string_view text) {
    // TODO: вернуть копию text, где ASCII-буквы переведены в верхний регистр.
    return std::string{text};  // неверно: ничего не меняем
}

}  // namespace textstats
