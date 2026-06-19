#include "geometry/textstats.hpp"
#include <cctype>   // std::isspace, std::toupper

namespace textstats {

int word_count(std::string_view text) {
    int count = 0;
    bool in_word = false;
    for (unsigned char ch : text) {
        if (std::isspace(ch)) {
            in_word = false;
        } else if (!in_word) {
            in_word = true;
            ++count;
        }
    }
    return count;
}

int longest_word(std::string_view text) {
    int best = 0;
    int current = 0;
    for (unsigned char ch : text) {
        if (std::isspace(ch)) {
            current = 0;
        } else {
            ++current;
            if (current > best) {
                best = current;
            }
        }
    }
    return best;
}

std::string to_upper(std::string_view text) {
    std::string result{text};
    for (char& ch : result) {
        ch = static_cast<char>(std::toupper(static_cast<unsigned char>(ch)));
    }
    return result;
}

}  // namespace textstats
