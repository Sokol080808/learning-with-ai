// Эти тесты трогать не нужно — это эталон поведения.
// Если хоть один красный — чини src/memory.c, а не этот файл.

#include <gtest/gtest.h>
#include <cstdint>   // uintptr_t — для аккуратной проверки выравнивания

extern "C" {
#include "memory.h"
}

// --- align_up -------------------------------------------------------------

TEST(AlignUp, RoundsUpToBoundary) {
    EXPECT_EQ(align_up(13, 8), 16u);
    EXPECT_EQ(align_up(1, 8), 8u);
    EXPECT_EQ(align_up(7, 8), 8u);
    EXPECT_EQ(align_up(9, 8), 16u);
}

TEST(AlignUp, AlreadyAlignedStaysSame) {
    EXPECT_EQ(align_up(0, 8), 0u);
    EXPECT_EQ(align_up(8, 8), 8u);
    EXPECT_EQ(align_up(16, 8), 16u);
    EXPECT_EQ(align_up(64, 16), 64u);
}

TEST(AlignUp, OtherPowersOfTwo) {
    EXPECT_EQ(align_up(5, 1), 5u);    // выравнивание по 1 ничего не меняет
    EXPECT_EQ(align_up(3, 4), 4u);
    EXPECT_EQ(align_up(17, 16), 32u);
    EXPECT_EQ(align_up(1000, 64), 1024u);
}

// --- is_aligned -----------------------------------------------------------

TEST(IsAligned, AlignByOneAlwaysTrue) {
    int x = 0;
    EXPECT_EQ(is_aligned(&x, 1), 1);
}

TEST(IsAligned, IntIsAlignedToFour) {
    // Компилятор размещает int на адресе, кратном alignof(int) == 4.
    int x = 0;
    EXPECT_EQ(is_aligned(&x, 4), 1);
}

TEST(IsAligned, DetectsMisaligned) {
    // Берём байтовый буфер и сдвигаем указатель на 1 байт — он гарантированно
    // НЕ кратен 2. А сам буфер выровняем явно, чтобы старт был чётным.
    alignas(8) unsigned char buf[8] = {0};
    const void* base   = &buf[0];   // адрес кратен 8 -> и 2 тоже
    const void* shift1 = &buf[1];   // на 1 байт правее -> нечётный

    EXPECT_EQ(is_aligned(base, 2), 1);
    EXPECT_EQ(is_aligned(shift1, 2), 0);
}

TEST(IsAligned, MatchesLowBitsOfAddress) {
    // Независимая проверка: ответ функции должен совпадать с "младшие биты адреса нули".
    alignas(16) unsigned char buf[16] = {0};
    for (size_t i = 0; i < sizeof(buf); ++i) {
        uintptr_t addr = reinterpret_cast<uintptr_t>(&buf[i]);
        int expected8 = (addr % 8 == 0) ? 1 : 0;
        EXPECT_EQ(is_aligned(&buf[i], 8), expected8) << "i=" << i;
    }
}

// --- nth_element ----------------------------------------------------------

TEST(NthElement, ReturnsAddressOfElement) {
    int arr[5] = {10, 20, 30, 40, 50};
    EXPECT_EQ(nth_element(arr, 0), &arr[0]);
    EXPECT_EQ(nth_element(arr, 2), &arr[2]);
    EXPECT_EQ(nth_element(arr, 4), &arr[4]);
}

TEST(NthElement, PointsToCorrectValue) {
    int arr[5] = {10, 20, 30, 40, 50};
    // Сначала убеждаемся, что вернулся НЕ NULL: ASSERT прерывает только этот
    // тест, поэтому заглушка `return NULL;` даёт честный красный, а не краш.
    int* p3 = nth_element(arr, 3);
    ASSERT_NE(p3, nullptr);
    EXPECT_EQ(*p3, 40);
    int* p0 = nth_element(arr, 0);
    ASSERT_NE(p0, nullptr);
    EXPECT_EQ(*p0, 10);
}

TEST(NthElement, MovesByElementsNotBytes) {
    // Разница адресов между соседними элементами == sizeof(int) байт.
    int arr[3] = {1, 2, 3};
    char* a0 = reinterpret_cast<char*>(nth_element(arr, 0));
    char* a1 = reinterpret_cast<char*>(nth_element(arr, 1));
    EXPECT_EQ(a1 - a0, static_cast<long>(sizeof(int)));
}

// --- ptr_distance ---------------------------------------------------------

TEST(PtrDistance, CountsElementsForward) {
    int arr[10] = {0};
    EXPECT_EQ(ptr_distance(&arr[5], &arr[2]), 3);
    EXPECT_EQ(ptr_distance(&arr[9], &arr[0]), 9);
}

TEST(PtrDistance, ZeroWhenSame) {
    int arr[4] = {0};
    EXPECT_EQ(ptr_distance(&arr[2], &arr[2]), 0);
    EXPECT_EQ(ptr_distance(&arr[0], &arr[0]), 0);
    // Контрольная не-нулевая проверка в том же кейсе: иначе заглушка
    // `return 0;` дала бы ложно-зелёный результат.
    EXPECT_EQ(ptr_distance(&arr[3], &arr[1]), 2);
}

TEST(PtrDistance, NegativeWhenBackward) {
    int arr[10] = {0};
    EXPECT_EQ(ptr_distance(&arr[1], &arr[7]), -6);
}

TEST(PtrDistance, ConsistentWithNthElement) {
    int arr[8] = {0};
    int* a = nth_element(arr, 6);
    int* b = nth_element(arr, 1);
    EXPECT_EQ(ptr_distance(a, b), 5);
}
