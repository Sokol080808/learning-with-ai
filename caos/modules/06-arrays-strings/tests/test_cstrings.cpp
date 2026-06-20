// Эти тесты трогать не нужно — это эталон поведения.
//
// Они описывают, как ДОЛЖНЫ работать твои функции из src/cstrings.c.
// Пока функции — заглушки, тесты красные. Когда реализуешь правильно — станут зелёными.
//
// Запуск:  ./caos/run.sh 06

#include <gtest/gtest.h>
#include <cstring>   // для проверочных сравнений в тестах (НЕ для твоего .c)
#include <random>
#include <string>
#include <algorithm>
#include <vector>

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

// ============================================================
//  RANDOMIZED / PROPERTY-BASED TESTS  (seeded mt19937 rng)
// ============================================================

// Helper: build a random printable string of given length using rng.
static std::string rand_str(std::mt19937& rng, size_t len) {
    // printable ASCII 0x20..0x7E; avoid '\0' inside the string.
    std::uniform_int_distribution<int> dist(0x21, 0x7E); // '!' to '~'
    std::string s(len, '\0');
    for (size_t i = 0; i < len; ++i) s[i] = static_cast<char>(dist(rng));
    return s;
}

// -------- my_strlen randomized oracle --------

TEST(MyStrlenRand, MatchesStdStrlenOnManyInputs) {
    std::mt19937 rng(0xC0FFEE);
    std::uniform_int_distribution<size_t> len_dist(0, 128);

    for (int iter = 0; iter < 1000; ++iter) {
        std::string s = rand_str(rng, len_dist(rng));
        // Oracle: std::strlen on the same null-terminated data.
        ASSERT_EQ(my_strlen(s.c_str()), std::strlen(s.c_str()))
            << "Failed for string: " << s;
    }
}

TEST(MyStrlenRand, StopsAtFirstNul) {
    // String with an embedded '\0' at position mid: my_strlen must return mid.
    std::mt19937 rng(0xDEAD);
    std::uniform_int_distribution<size_t> len_dist(2, 64);

    for (int iter = 0; iter < 500; ++iter) {
        size_t len = len_dist(rng);
        std::string s = rand_str(rng, len); // no '\0' inside
        // plant a '\0' at a random interior position
        std::uniform_int_distribution<size_t> pos_dist(0, len - 1);
        size_t nul_pos = pos_dist(rng);
        s[nul_pos] = '\0';
        // my_strlen should return exactly nul_pos
        ASSERT_EQ(my_strlen(s.data()), nul_pos)
            << "Embedded NUL at " << nul_pos << " but strlen walked past it";
    }
}

TEST(MyStrlenRand, EdgeLengths) {
    // Explicit edge: length 0 and length 1.
    EXPECT_EQ(my_strlen(""), 0u);
    char one[2] = {'x', '\0'};
    EXPECT_EQ(my_strlen(one), 1u);
    // Length exactly 255 (boundary for single byte).
    std::string big(255, 'A');
    EXPECT_EQ(my_strlen(big.c_str()), 255u);
}

// -------- safe_copy randomized oracle --------

TEST(SafeCopyRand, ReturnValueIsAlwaysSrcLen) {
    // safe_copy must return my_strlen(src) regardless of cap.
    std::mt19937 rng(0xBEEF01);
    std::uniform_int_distribution<size_t> slen_dist(0, 80);
    std::uniform_int_distribution<size_t> cap_dist(1, 64);

    for (int iter = 0; iter < 800; ++iter) {
        std::string src = rand_str(rng, slen_dist(rng));
        size_t cap = cap_dist(rng);
        std::vector<char> dst(cap + 4, '\xAB'); // poison extra bytes
        size_t ret = safe_copy(dst.data(), cap, src.c_str());
        ASSERT_EQ(ret, src.size())
            << "safe_copy returned " << ret << " but src len is " << src.size();
    }
}

TEST(SafeCopyRand, AlwaysNulTerminates) {
    // dst[cap-1] must be '\0' after any call with cap >= 1.
    std::mt19937 rng(0xBEEF02);
    std::uniform_int_distribution<size_t> slen_dist(0, 80);
    std::uniform_int_distribution<size_t> cap_dist(1, 64);

    for (int iter = 0; iter < 800; ++iter) {
        std::string src = rand_str(rng, slen_dist(rng));
        size_t cap = cap_dist(rng);
        std::vector<char> dst(cap + 4, '\xCC');
        safe_copy(dst.data(), cap, src.c_str());
        // The result must be a valid C-string of length <= cap-1.
        size_t result_len = std::strlen(dst.data());
        ASSERT_LE(result_len, cap - 1)
            << "Result length " << result_len << " exceeds cap-1=" << cap-1;
    }
}

TEST(SafeCopyRand, CopiesCorrectContent) {
    // When src fits in cap: dst must equal src exactly.
    // When truncated: dst must be a prefix of src of length cap-1.
    std::mt19937 rng(0xBEEF03);
    std::uniform_int_distribution<size_t> slen_dist(0, 60);
    std::uniform_int_distribution<size_t> cap_dist(1, 80);

    for (int iter = 0; iter < 800; ++iter) {
        std::string src = rand_str(rng, slen_dist(rng));
        size_t cap = cap_dist(rng);
        std::vector<char> dst(cap + 8, '\xCC');
        safe_copy(dst.data(), cap, src.c_str());

        std::string result(dst.data()); // reads up to first '\0'
        if (src.size() < cap) {
            // No truncation: full copy.
            ASSERT_EQ(result, src)
                << "Full copy mismatch: src='" << src << "' cap=" << cap;
        } else {
            // Truncated: must be exactly the first cap-1 chars of src.
            ASSERT_EQ(result, src.substr(0, cap - 1))
                << "Truncated copy mismatch: src='" << src << "' cap=" << cap;
        }
    }
}

TEST(SafeCopyRand, TruncationDetectable) {
    // Invariant: return >= cap <=> truncation occurred.
    std::mt19937 rng(0xBEEF04);
    std::uniform_int_distribution<size_t> slen_dist(0, 80);
    std::uniform_int_distribution<size_t> cap_dist(1, 64);

    for (int iter = 0; iter < 800; ++iter) {
        std::string src = rand_str(rng, slen_dist(rng));
        size_t cap = cap_dist(rng);
        std::vector<char> dst(cap + 8, '\xCC');
        size_t ret = safe_copy(dst.data(), cap, src.c_str());

        bool was_truncated = (src.size() >= cap);
        bool ret_signals_truncation = (ret >= cap);
        ASSERT_EQ(was_truncated, ret_signals_truncation)
            << "src.size()=" << src.size() << " cap=" << cap << " ret=" << ret;
    }
}

TEST(SafeCopyRand, CapZeroNoWrite) {
    // cap==0: must not write anything; return src length.
    std::mt19937 rng(0xBEEF05);
    std::uniform_int_distribution<size_t> slen_dist(1, 50);

    for (int iter = 0; iter < 500; ++iter) {
        std::string src = rand_str(rng, slen_dist(rng));
        // Use a canary byte before and after the dst pointer.
        char canary[4] = {'\xAA', '\xAA', '\xAA', '\xAA'};
        // Pass pointer to middle byte — any write would corrupt canary.
        // We can only safely test no-write by checking return value and
        // verifying canary is untouched. Point dst at canary[1]; cap=0.
        char* dst = canary + 1;
        size_t ret = safe_copy(dst, 0, src.c_str());
        ASSERT_EQ(ret, src.size()) << "cap=0 must still return src length";
        ASSERT_EQ(canary[0], '\xAA') << "cap=0 wrote before dst";
        ASSERT_EQ(canary[1], '\xAA') << "cap=0 wrote into dst";
        ASSERT_EQ(canary[2], '\xAA') << "cap=0 wrote after dst";
    }
}

// -------- reverse_string randomized properties --------

TEST(ReverseStringRand, DoubleReverseIsIdentity) {
    // Reversing twice must return the original string.
    std::mt19937 rng(0xFACE01);
    std::uniform_int_distribution<size_t> len_dist(0, 100);

    for (int iter = 0; iter < 1000; ++iter) {
        std::string s = rand_str(rng, len_dist(rng));
        std::string original = s;
        std::vector<char> buf(s.begin(), s.end());
        buf.push_back('\0');
        reverse_string(buf.data());
        reverse_string(buf.data());
        ASSERT_STREQ(buf.data(), original.c_str())
            << "Double-reverse != original for: " << original;
    }
}

TEST(ReverseStringRand, MatchesStdReverse) {
    // Single reverse must match std::reverse on a copy.
    std::mt19937 rng(0xFACE02);
    std::uniform_int_distribution<size_t> len_dist(0, 100);

    for (int iter = 0; iter < 1000; ++iter) {
        std::string s = rand_str(rng, len_dist(rng));
        // Oracle: std::reverse
        std::string expected = s;
        std::reverse(expected.begin(), expected.end());

        std::vector<char> buf(s.begin(), s.end());
        buf.push_back('\0');
        reverse_string(buf.data());
        ASSERT_STREQ(buf.data(), expected.c_str())
            << "reverse_string mismatch for: " << s;
    }
}

TEST(ReverseStringRand, NulTerminatorPreserved) {
    // The '\0' must remain at position length, not earlier or later.
    std::mt19937 rng(0xFACE03);
    std::uniform_int_distribution<size_t> len_dist(1, 80);

    for (int iter = 0; iter < 500; ++iter) {
        std::string s = rand_str(rng, len_dist(rng));
        std::vector<char> buf(s.begin(), s.end());
        buf.push_back('\0');
        // Poison the byte right after '\0'.
        buf.push_back('\xBB');
        reverse_string(buf.data());
        // '\0' must still be at index s.size().
        ASSERT_EQ(buf[s.size()], '\0')
            << "NUL terminator moved after reverse_string";
        // Poison byte must be untouched.
        ASSERT_EQ((unsigned char)buf[s.size() + 1], 0xBBu)
            << "reverse_string wrote past the NUL terminator";
    }
}

TEST(ReverseStringRand, LengthUnchanged) {
    // Reversing must not change the visible length.
    std::mt19937 rng(0xFACE04);
    std::uniform_int_distribution<size_t> len_dist(0, 100);

    for (int iter = 0; iter < 500; ++iter) {
        std::string s = rand_str(rng, len_dist(rng));
        std::vector<char> buf(s.begin(), s.end());
        buf.push_back('\0');
        reverse_string(buf.data());
        ASSERT_EQ(std::strlen(buf.data()), s.size())
            << "Length changed after reverse_string";
    }
}

// -------- my_strcmp randomized oracle --------

TEST(MyStrcmpRand, SignMatchesStdStrcmp) {
    // The sign of my_strcmp must match the sign of std::strcmp for all inputs.
    auto sign = [](int v) -> int { return (v > 0) - (v < 0); };

    std::mt19937 rng(0xC0DE01);
    std::uniform_int_distribution<size_t> len_dist(0, 50);

    for (int iter = 0; iter < 1000; ++iter) {
        std::string a = rand_str(rng, len_dist(rng));
        std::string b = rand_str(rng, len_dist(rng));
        int mine = my_strcmp(a.c_str(), b.c_str());
        int libc = std::strcmp(a.c_str(), b.c_str());
        ASSERT_EQ(sign(mine), sign(libc))
            << "my_strcmp(\"" << a << "\", \"" << b << "\") sign="
            << sign(mine) << " but strcmp sign=" << sign(libc);
    }
}

TEST(MyStrcmpRand, EqualStringsReturnZero) {
    // my_strcmp(s, s) == 0 for any string s.
    std::mt19937 rng(0xC0DE02);
    std::uniform_int_distribution<size_t> len_dist(0, 80);

    for (int iter = 0; iter < 1000; ++iter) {
        std::string s = rand_str(rng, len_dist(rng));
        ASSERT_EQ(my_strcmp(s.c_str(), s.c_str()), 0)
            << "my_strcmp(s, s) != 0 for s=\"" << s << "\"";
    }
}

TEST(MyStrcmpRand, Antisymmetry) {
    // sign(my_strcmp(a, b)) == -sign(my_strcmp(b, a))  (strict antisymmetry).
    auto sign = [](int v) -> int { return (v > 0) - (v < 0); };

    std::mt19937 rng(0xC0DE03);
    std::uniform_int_distribution<size_t> len_dist(0, 50);

    for (int iter = 0; iter < 1000; ++iter) {
        std::string a = rand_str(rng, len_dist(rng));
        std::string b = rand_str(rng, len_dist(rng));
        if (a == b) continue; // skip equal pairs for the antisymmetry check
        int ab = sign(my_strcmp(a.c_str(), b.c_str()));
        int ba = sign(my_strcmp(b.c_str(), a.c_str()));
        ASSERT_EQ(ab, -ba)
            << "Antisymmetry violated: strcmp(a,b)=" << ab
            << " strcmp(b,a)=" << ba
            << " a=\"" << a << "\" b=\"" << b << "\"";
    }
}

TEST(MyStrcmpRand, UnsignedByteOrdering) {
    // All high-byte strings (0x80..0xFF) must sort after all low-byte strings.
    // Oracle: std::strcmp, which uses unsigned char comparison.
    auto sign = [](int v) -> int { return (v > 0) - (v < 0); };

    std::mt19937 rng(0xC0DE04);
    std::uniform_int_distribution<int> lo_dist(0x01, 0x7F);
    std::uniform_int_distribution<int> hi_dist(0x80, 0xFF);
    std::uniform_int_distribution<size_t> len_dist(1, 20);

    for (int iter = 0; iter < 500; ++iter) {
        size_t la = len_dist(rng), lb = len_dist(rng);
        std::string a(la, '\0'), b(lb, '\0');
        for (size_t i = 0; i < la; ++i) a[i] = static_cast<char>(lo_dist(rng));
        for (size_t i = 0; i < lb; ++i) b[i] = static_cast<char>(hi_dist(rng));

        int mine = my_strcmp(a.c_str(), b.c_str());
        int libc = std::strcmp(a.c_str(), b.c_str());
        ASSERT_EQ(sign(mine), sign(libc))
            << "High-byte ordering mismatch";
    }
}

TEST(MyStrcmpRand, PrefixOrdering) {
    // If a is a strict prefix of b, my_strcmp(a, b) < 0.
    std::mt19937 rng(0xC0DE05);
    std::uniform_int_distribution<size_t> len_dist(1, 60);

    for (int iter = 0; iter < 500; ++iter) {
        std::string b = rand_str(rng, len_dist(rng));
        // a is a random-length strict prefix of b (length 0..len-1).
        std::uniform_int_distribution<size_t> plen_dist(0, b.size() - 1);
        std::string a = b.substr(0, plen_dist(rng));
        ASSERT_LT(my_strcmp(a.c_str(), b.c_str()), 0)
            << "Prefix \"" << a << "\" should compare less than \"" << b << "\"";
        ASSERT_GT(my_strcmp(b.c_str(), a.c_str()), 0)
            << "\"" << b << "\" should compare greater than prefix \"" << a << "\"";
    }
}

// ============================================================
//  StrBuf tests
// ============================================================

// ---------- strbuf_init ----------

TEST(StrBuf, InitBasic) {
    StrBuf sb;
    strbuf_init(&sb, 32);
    ASSERT_NE(sb.buf, nullptr);
    EXPECT_EQ(sb.len, 0u);
    EXPECT_EQ(sb.cap, 32u);
    EXPECT_STREQ(sb.buf, "");
    strbuf_free(&sb);
}

TEST(StrBuf, InitCapZero) {
    StrBuf sb;
    strbuf_init(&sb, 0);
    EXPECT_EQ(sb.buf, nullptr);
    EXPECT_EQ(sb.len, 0u);
    EXPECT_EQ(sb.cap, 0u);
    // strbuf_free on a zero-cap buf must be safe
    strbuf_free(&sb);
}

TEST(StrBuf, InitCapOne) {
    // cap=1: room for '\0' only — no actual characters can be stored.
    StrBuf sb;
    strbuf_init(&sb, 1);
    ASSERT_NE(sb.buf, nullptr);
    EXPECT_EQ(sb.len, 0u);
    EXPECT_EQ(sb.cap, 1u);
    EXPECT_STREQ(sb.buf, "");
    strbuf_free(&sb);
}

// ---------- strbuf_append ----------

TEST(StrBuf, AppendFits) {
    StrBuf sb;
    strbuf_init(&sb, 16);
    size_t written = strbuf_append(&sb, "hello");
    EXPECT_EQ(written, 5u);
    EXPECT_EQ(sb.len, 5u);
    EXPECT_STREQ(sb.buf, "hello");
    strbuf_free(&sb);
}

TEST(StrBuf, AppendEmptyString) {
    StrBuf sb;
    strbuf_init(&sb, 16);
    size_t written = strbuf_append(&sb, "");
    EXPECT_EQ(written, 0u);
    EXPECT_EQ(sb.len, 0u);
    EXPECT_STREQ(sb.buf, "");
    strbuf_free(&sb);
}

TEST(StrBuf, AppendTruncates) {
    // cap=8: room for 7 chars + '\0'. "overflow" has 8 chars — must be truncated to 7.
    StrBuf sb;
    strbuf_init(&sb, 8);
    size_t written = strbuf_append(&sb, "overflow");
    EXPECT_EQ(written, 7u);          // only 7 chars fit (cap-1)
    EXPECT_EQ(sb.len, 7u);
    EXPECT_EQ(std::strlen(sb.buf), 7u);
    EXPECT_STREQ(sb.buf, "overflo");
    strbuf_free(&sb);
}

TEST(StrBuf, AppendAlwaysNulTerminates) {
    // Even after a truncating append, buf must be NUL-terminated.
    StrBuf sb;
    strbuf_init(&sb, 4);
    strbuf_append(&sb, "abcdef");
    // buf must be valid C-string of length <= 3
    EXPECT_EQ(std::strlen(sb.buf), 3u);
    EXPECT_EQ(sb.buf[3], '\0');
    strbuf_free(&sb);
}

TEST(StrBuf, AppendAccumulates) {
    // Multiple appends must concatenate content.
    StrBuf sb;
    strbuf_init(&sb, 32);
    strbuf_append(&sb, "foo");
    strbuf_append(&sb, "bar");
    strbuf_append(&sb, "baz");
    EXPECT_EQ(sb.len, 9u);
    EXPECT_STREQ(sb.buf, "foobarbaz");
    strbuf_free(&sb);
}

TEST(StrBuf, AppendAccumulatesWithTruncation) {
    // Fill the buffer step by step; last append must be truncated.
    StrBuf sb;
    strbuf_init(&sb, 8);   // 7 usable chars
    size_t w1 = strbuf_append(&sb, "abc");   // fits: 3 chars
    size_t w2 = strbuf_append(&sb, "defgh"); // only 4 chars fit (7-3=4), "defg" written
    EXPECT_EQ(w1, 3u);
    EXPECT_EQ(w2, 4u);
    EXPECT_EQ(sb.len, 7u);
    EXPECT_STREQ(sb.buf, "abcdefg");
    strbuf_free(&sb);
}

TEST(StrBuf, AppendToFullBufferReturnsZero) {
    // Once the buffer is full, further appends return 0.
    StrBuf sb;
    strbuf_init(&sb, 4);   // 3 usable chars
    strbuf_append(&sb, "abc");
    EXPECT_EQ(sb.len, 3u);
    size_t written = strbuf_append(&sb, "more");
    EXPECT_EQ(written, 0u);
    EXPECT_EQ(sb.len, 3u);
    EXPECT_STREQ(sb.buf, "abc");
    strbuf_free(&sb);
}

TEST(StrBuf, AppendCapOneBehavior) {
    // cap=1: only '\0' fits. Every append returns 0.
    StrBuf sb;
    strbuf_init(&sb, 1);
    size_t written = strbuf_append(&sb, "hello");
    EXPECT_EQ(written, 0u);
    EXPECT_STREQ(sb.buf, "");
    strbuf_free(&sb);
}

// ---------- strbuf_clear ----------

TEST(StrBuf, ClearResetsContent) {
    StrBuf sb;
    strbuf_init(&sb, 16);
    strbuf_append(&sb, "hello");
    strbuf_clear(&sb);
    EXPECT_EQ(sb.len, 0u);
    EXPECT_STREQ(sb.buf, "");
    // After clear, cap must remain unchanged.
    EXPECT_EQ(sb.cap, 16u);
    strbuf_free(&sb);
}

TEST(StrBuf, ClearAllowsReuse) {
    // After clear, the buffer should be fully reusable.
    StrBuf sb;
    strbuf_init(&sb, 8);
    strbuf_append(&sb, "abcdef");
    strbuf_clear(&sb);
    strbuf_append(&sb, "xyz");
    EXPECT_EQ(sb.len, 3u);
    EXPECT_STREQ(sb.buf, "xyz");
    strbuf_free(&sb);
}

// ---------- strbuf_free ----------

TEST(StrBuf, FreeZerosFields) {
    StrBuf sb;
    strbuf_init(&sb, 32);
    strbuf_append(&sb, "data");
    strbuf_free(&sb);
    EXPECT_EQ(sb.buf, nullptr);
    EXPECT_EQ(sb.len, 0u);
    EXPECT_EQ(sb.cap, 0u);
}

TEST(StrBuf, FreeOnNullBufIsSafe) {
    // Calling strbuf_free on an uninitialized/zeroed StrBuf must not crash.
    StrBuf sb = {nullptr, 0, 0};
    strbuf_free(&sb); // must not crash (free(NULL) is defined no-op)
}

// ---------- randomized StrBuf properties ----------

TEST(StrBufRand, AppendNeverExceedsCap) {
    // After any sequence of appends, len must always be < cap.
    std::mt19937 rng(0x5B00F01);
    std::uniform_int_distribution<size_t> cap_dist(1, 64);
    std::uniform_int_distribution<size_t> slen_dist(0, 80);
    std::uniform_int_distribution<int>    n_appends_dist(1, 10);

    for (int iter = 0; iter < 500; ++iter) {
        size_t cap = cap_dist(rng);
        StrBuf sb;
        strbuf_init(&sb, cap);
        if (sb.buf == nullptr) continue;

        int n = n_appends_dist(rng);
        for (int k = 0; k < n; ++k) {
            std::string s = rand_str(rng, slen_dist(rng));
            strbuf_append(&sb, s.c_str());
            ASSERT_LT(sb.len, sb.cap)
                << "len >= cap after append (iter=" << iter << " k=" << k << ")";
            ASSERT_EQ(sb.buf[sb.len], '\0')
                << "NUL missing after append";
        }
        strbuf_free(&sb);
    }
}

TEST(StrBufRand, AppendReturnedBytesMatchContent) {
    // The sum of returned bytes from all appends must equal final sb.len
    // (when no single append overflows in the middle of a call).
    std::mt19937 rng(0x5B00F02);
    std::uniform_int_distribution<size_t> cap_dist(4, 128);
    std::uniform_int_distribution<size_t> slen_dist(0, 30);

    for (int iter = 0; iter < 500; ++iter) {
        size_t cap = cap_dist(rng);
        StrBuf sb;
        strbuf_init(&sb, cap);
        if (sb.buf == nullptr) continue;

        size_t total_written = 0;
        for (int k = 0; k < 8; ++k) {
            std::string s = rand_str(rng, slen_dist(rng));
            total_written += strbuf_append(&sb, s.c_str());
        }
        ASSERT_EQ(total_written, sb.len)
            << "Sum of returned bytes != sb.len (iter=" << iter << ")";
        strbuf_free(&sb);
    }
}

TEST(StrBufRand, ContentMatchesManualConcat) {
    // Build the same string manually and compare to StrBuf content.
    std::mt19937 rng(0x5B00F03);
    std::uniform_int_distribution<size_t> cap_dist(8, 128);
    std::uniform_int_distribution<size_t> slen_dist(0, 40);

    for (int iter = 0; iter < 300; ++iter) {
        size_t cap = cap_dist(rng);
        StrBuf sb;
        strbuf_init(&sb, cap);
        if (sb.buf == nullptr) continue;

        std::string expected;
        for (int k = 0; k < 6; ++k) {
            std::string s = rand_str(rng, slen_dist(rng));
            strbuf_append(&sb, s.c_str());
            expected += s;
        }
        // Truncate expected to cap-1 chars (what the buffer allows).
        if (expected.size() >= cap) {
            expected = expected.substr(0, cap - 1);
        }
        ASSERT_STREQ(sb.buf, expected.c_str())
            << "Content mismatch at iter=" << iter << " cap=" << cap;
        strbuf_free(&sb);
    }
}

TEST(StrBufRand, ClearAndRefill) {
    // After strbuf_clear the buffer is reusable and behaves identically.
    std::mt19937 rng(0x5B00F04);
    std::uniform_int_distribution<size_t> cap_dist(4, 64);
    std::uniform_int_distribution<size_t> slen_dist(0, 40);

    for (int iter = 0; iter < 300; ++iter) {
        size_t cap = cap_dist(rng);
        StrBuf sb;
        strbuf_init(&sb, cap);
        if (sb.buf == nullptr) continue;

        // First fill
        std::string s1 = rand_str(rng, slen_dist(rng));
        strbuf_append(&sb, s1.c_str());

        strbuf_clear(&sb);
        ASSERT_EQ(sb.len, 0u);
        ASSERT_EQ(sb.cap, cap);

        // Second fill — must behave exactly as a fresh init
        std::string s2 = rand_str(rng, slen_dist(rng));
        strbuf_append(&sb, s2.c_str());

        std::string expected = s2.size() < cap ? s2 : s2.substr(0, cap - 1);
        ASSERT_STREQ(sb.buf, expected.c_str())
            << "After clear+refill mismatch at iter=" << iter;
        strbuf_free(&sb);
    }
}
