// Эти тесты трогать не нужно — это эталон поведения.
//
// Они описывают, как ДОЛЖНЫ работать твои функции из src/cstrings.c.
// Пока функции — заглушки, тесты красные. Когда реализуешь правильно — станут зелёными.
//
// Запуск:  ./caos/run.sh 06

#include <gtest/gtest.h>
#include <cstring>   // для проверочных сравнений в тестах (НЕ для твоего .c)

extern "C" {
#include "cstrings.h"
}

// ---------- my_strlen ----------

TEST(MyStrlen, EmptyString) {
    EXPECT_EQ(my_strlen(""), 0u);
}

TEST(MyStrlen, SimpleWords) {
    EXPECT_EQ(my_strlen("a"), 1u);
    EXPECT_EQ(my_strlen("hello"), 5u);
    EXPECT_EQ(my_strlen("computer architecture"), 21u);
}

TEST(MyStrlen, StopsAtNulByte) {
    // В буфере после '\0' лежит мусор — длина считается ДО '\0'.
    const char buf[] = {'h', 'i', '\0', 'X', 'Y'};
    EXPECT_EQ(my_strlen(buf), 2u);
}

TEST(MyStrlen, MatchesLibc) {
    EXPECT_EQ(my_strlen("any string here"), std::strlen("any string here"));
}

// ---------- safe_copy ----------

TEST(SafeCopy, FitsExactly) {
    char dst[16];
    size_t n = safe_copy(dst, sizeof(dst), "hello");
    EXPECT_EQ(n, 5u);              // вернули длину src
    EXPECT_STREQ(dst, "hello");    // скопировали целиком и завершили '\0'
}

TEST(SafeCopy, EmptySource) {
    char dst[8];
    size_t n = safe_copy(dst, sizeof(dst), "");
    EXPECT_EQ(n, 0u);
    EXPECT_STREQ(dst, "");
}

TEST(SafeCopy, TruncatesToCapMinusOne) {
    // cap = 4 -> копируем максимум 3 символа, плюс '\0'.
    char dst[4];
    size_t n = safe_copy(dst, sizeof(dst), "abcdef");
    EXPECT_EQ(n, 6u);              // вернули полную длину src (для детекта усечения)
    EXPECT_STREQ(dst, "abc");      // в буфере — только то, что влезло
}

TEST(SafeCopy, AlwaysNulTerminates) {
    // Заранее «грязный» буфер: каждый байт != 0. Если copy не поставит '\0',
    // strlen уедет за границы. Корректная реализация всегда завершает строку.
    char dst[5];
    std::memset(dst, 'Z', sizeof(dst));
    safe_copy(dst, sizeof(dst), "wxyz_overflow");
    EXPECT_EQ(std::strlen(dst), 4u);  // ровно cap-1 символов
    EXPECT_STREQ(dst, "wxyz");
}

TEST(SafeCopy, CapOneGivesEmptyString) {
    // cap = 1 -> места ровно под '\0', сам контент не влезает.
    char dst[1];
    std::memset(dst, 'Z', sizeof(dst));
    size_t n = safe_copy(dst, sizeof(dst), "data");
    EXPECT_EQ(n, 4u);              // длина src всё равно возвращается
    EXPECT_STREQ(dst, "");         // буфер — пустая строка
}

TEST(SafeCopy, TruncationDetectableByReturn) {
    char dst[4];
    size_t n = safe_copy(dst, sizeof(dst), "abcdef");
    // Контракт: усечение произошло <=> возвращённая длина >= cap.
    EXPECT_GE(n, sizeof(dst));
}

// ---------- reverse_string ----------

TEST(ReverseString, OddLength) {
    char s[] = "abc";
    reverse_string(s);
    EXPECT_STREQ(s, "cba");
}

TEST(ReverseString, EvenLength) {
    char s[] = "abcd";
    reverse_string(s);
    EXPECT_STREQ(s, "dcba");
}

TEST(ReverseString, SingleChar) {
    char s[] = "x";
    reverse_string(s);
    EXPECT_STREQ(s, "x");
}

TEST(ReverseString, EmptyString) {
    char s[] = "";
    reverse_string(s);
    EXPECT_STREQ(s, "");
}

TEST(ReverseString, Palindrome) {
    char s[] = "level";
    reverse_string(s);
    EXPECT_STREQ(s, "level");
}

TEST(ReverseString, WithSpaces) {
    char s[] = "ab cd";
    reverse_string(s);
    EXPECT_STREQ(s, "dc ba");
}

// ---------- my_strcmp ----------

TEST(MyStrcmp, EqualStrings) {
    EXPECT_EQ(my_strcmp("hello", "hello"), 0);
    EXPECT_EQ(my_strcmp("", ""), 0);
}

TEST(MyStrcmp, FirstLessSecond) {
    EXPECT_LT(my_strcmp("abc", "abd"), 0);
    EXPECT_LT(my_strcmp("abc", "abcd"), 0);  // префикс короче
    EXPECT_LT(my_strcmp("", "a"), 0);
}

TEST(MyStrcmp, FirstGreaterSecond) {
    EXPECT_GT(my_strcmp("abd", "abc"), 0);
    EXPECT_GT(my_strcmp("abcd", "abc"), 0);  // длиннее на префиксе
    EXPECT_GT(my_strcmp("a", ""), 0);
}

TEST(MyStrcmp, SignMatchesLibc) {
    // Сравниваем только ЗНАК с настоящим strcmp (величина не нормирована стандартом).
    auto sign = [](int v) { return (v > 0) - (v < 0); };
    EXPECT_EQ(sign(my_strcmp("apple", "banana")), sign(std::strcmp("apple", "banana")));
    EXPECT_EQ(sign(my_strcmp("zoo", "ant")),       sign(std::strcmp("zoo", "ant")));
    EXPECT_EQ(sign(my_strcmp("cat", "cat")),       sign(std::strcmp("cat", "cat")));
}

TEST(MyStrcmp, UnsignedCharSemantics) {
    // Байты сравниваются как unsigned char (как в настоящем strcmp).
    // '\x80' (128) больше, чем 'a' (97).
    const char hi[]  = {'\x80', '\0'};
    const char lo[]  = {'a', '\0'};
    EXPECT_GT(my_strcmp(hi, lo), 0);
}
