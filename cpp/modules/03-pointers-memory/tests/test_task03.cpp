#include <gtest/gtest.h>
#include <stdexcept>
#include <random>
#include <vector>
#include <algorithm>
#include <numeric>
#include "task03.hpp"

TEST(Pointers, SumArray) {
    int a[] = {1, 2, 3, 4, 5};
    EXPECT_EQ(sum_array(a, 5), 15);
    EXPECT_EQ(sum_array(a, 0), 0);
    int b[] = {-3, 3};
    EXPECT_EQ(sum_array(b, 2), 0);
}

TEST(Pointers, SwapInts) {
    int x = 1, y = 2;
    swap_ints(&x, &y);
    EXPECT_EQ(x, 2);
    EXPECT_EQ(y, 1);
}

TEST(Pointers, CountValue) {
    int a[] = {1, 2, 2, 3, 2};
    EXPECT_EQ(count_value(a, 5, 2), 3);
    EXPECT_EQ(count_value(a, 5, 9), 0);
}

TEST(Pointers, MaxElementPtr) {
    int a[] = {3, 7, 1, 9, 4};
    const int* m = max_element_ptr(a, 5);
    ASSERT_NE(m, nullptr);
    EXPECT_EQ(*m, 9);
    EXPECT_EQ(m, &a[3]);  // именно указатель на нужный элемент
    EXPECT_EQ(max_element_ptr(a, 0), nullptr);
}

TEST(Pointers, ReverseInPlace) {
    int a[] = {1, 2, 3, 4, 5};
    reverse_in_place(a, 5);
    EXPECT_EQ(a[0], 5);
    EXPECT_EQ(a[1], 4);
    EXPECT_EQ(a[2], 3);
    EXPECT_EQ(a[3], 2);
    EXPECT_EQ(a[4], 1);

    int b[] = {10, 20};
    reverse_in_place(b, 2);
    EXPECT_EQ(b[0], 20);
    EXPECT_EQ(b[1], 10);
}

// ── Задание 6: reverse через два указателя ───────────────────────────────────

TEST(Pointers, ReverseWithPointersOdd) {
    int a[] = {1, 2, 3, 4, 5};
    reverse_with_pointers(a, 5);
    EXPECT_EQ(a[0], 5);
    EXPECT_EQ(a[1], 4);
    EXPECT_EQ(a[2], 3);
    EXPECT_EQ(a[3], 2);
    EXPECT_EQ(a[4], 1);
}

TEST(Pointers, ReverseWithPointersEven) {
    int a[] = {7, 8, 9, 10};
    reverse_with_pointers(a, 4);
    EXPECT_EQ(a[0], 10);
    EXPECT_EQ(a[1], 9);
    EXPECT_EQ(a[2], 8);
    EXPECT_EQ(a[3], 7);
}

TEST(Pointers, ReverseWithPointersEdgeCases) {
    // n == 1 — ничего не меняется.
    int one[] = {42};
    reverse_with_pointers(one, 1);
    EXPECT_EQ(one[0], 42);

    // n == 0 — не должно тронуть память (просто не падаем).
    int* empty = nullptr;
    reverse_with_pointers(empty, 0);
    SUCCEED();
}

// ── Задание 7: DynArray ──────────────────────────────────────────────────────

TEST(DynArrayTest, FillsOnConstruction) {
    DynArray a(4, 7);
    EXPECT_EQ(a.size(), 4);
    EXPECT_EQ(a.at(0), 7);
    EXPECT_EQ(a.at(1), 7);
    EXPECT_EQ(a.at(2), 7);
    EXPECT_EQ(a.at(3), 7);
}

TEST(DynArrayTest, DefaultFillIsZero) {
    DynArray a(3);
    EXPECT_EQ(a.size(), 3);
    EXPECT_EQ(a.sum(), 0);
}

TEST(DynArrayTest, EmptyArray) {
    DynArray a(0, 99);
    EXPECT_EQ(a.size(), 0);
    EXPECT_EQ(a.sum(), 0);
}

TEST(DynArrayTest, WriteThroughAtAndSum) {
    DynArray a(3, 0);
    a.at(0) = 10;   // неконстантная at возвращает int&
    a.at(1) = 20;
    a.at(2) = 30;
    EXPECT_EQ(a.at(1), 20);
    EXPECT_EQ(a.sum(), 60);
}

TEST(DynArrayTest, FillOverwritesAll) {
    DynArray a(5, 1);
    a.fill(3);
    EXPECT_EQ(a.sum(), 15);
    for (int i = 0; i < a.size(); ++i) {
        EXPECT_EQ(a.at(i), 3);
    }
}

TEST(DynArrayTest, AtThrowsOutOfRange) {
    DynArray a(2, 0);
    EXPECT_THROW(a.at(-1), std::out_of_range);
    EXPECT_THROW(a.at(2), std::out_of_range);
    EXPECT_THROW(a.at(100), std::out_of_range);
    // А корректный индекс не бросает.
    EXPECT_NO_THROW(a.at(0));
}

TEST(DynArrayTest, ConstAtReads) {
    const DynArray a(3, 5);
    EXPECT_EQ(a.at(2), 5);
    EXPECT_THROW(a.at(3), std::out_of_range);
}

// ── Задание 8: глубокое vs поверхностное копирование ─────────────────────────

TEST(OwnedIntTest, CopyConstructorIsDeepValue) {
    OwnedInt a(42);
    OwnedInt b(a);
    EXPECT_EQ(b.get(), 42);          // значение скопировано
    EXPECT_NE(a.raw(), b.raw());     // но буфер ОТДЕЛЬНЫЙ (глубокое копирование)
}

TEST(OwnedIntTest, CopyIsIndependent) {
    OwnedInt a(10);
    OwnedInt b(a);
    EXPECT_NE(a.raw(), b.raw());      // буферы разные — иначе set(99) задел бы оба
    b.set(99);
    EXPECT_EQ(a.get(), 10);          // изменение копии не задело оригинал
    EXPECT_EQ(b.get(), 99);
}

TEST(OwnedIntTest, AssignmentIsDeepValue) {
    OwnedInt a(5);
    OwnedInt b(0);
    b = a;
    EXPECT_EQ(b.get(), 5);
    EXPECT_NE(a.raw(), b.raw());     // присваивание тоже даёт свой буфер
}

TEST(OwnedIntTest, AssignmentIsIndependent) {
    OwnedInt a(7);
    OwnedInt b(0);
    b = a;
    a.set(123);                      // меняем оригинал после присваивания
    EXPECT_EQ(b.get(), 7);           // копия не должна измениться
}

TEST(OwnedIntTest, SelfAssignmentSafe) {
    OwnedInt a(3);
    a = a;                           // не должно ломать объект или течь
    EXPECT_EQ(a.get(), 3);
}

// ===== РАНДОМИЗИРОВАННЫЕ / PROPERTY-ТЕСТЫ (детерминированы фиксированным сидом) =====
//
// Эти тесты прогоняют МНОГО случайных входов (фиксированный сид std::mt19937,
// чтобы CI не флакал) и проверяют ИНВАРИАНТЫ, а не конкретные примеры:
//   - оракул против std:: (accumulate / count / max_element / reverse);
//   - reverse — инволюция и перестановка входа; два разворота согласованы;
//   - swap — обмен значений и идемпотентность двойного обмена;
//   - DynArray — заполнение, запись насквозь, sum против оракула, границы at();
//   - OwnedInt — глубокая копия: отдельный буфер и независимость значений.

// ── sum_array: оракул против std::accumulate, плюс крайние n ──────────────────
TEST(PointersProps, SumArrayMatchesAccumulate) {
    std::mt19937 rng(0xC0FFEEu);
    std::uniform_int_distribution<int> sz(0, 64);
    std::uniform_int_distribution<int> val(-1000, 1000);

    for (int iter = 0; iter < 400; ++iter) {
        int n = sz(rng);
        std::vector<int> v(n);
        for (int& x : v) x = val(rng);

        // Оракул: long long, чтобы не путать переполнением сам тест.
        long long oracle = 0;
        for (int x : v) oracle += x;

        const int* p = v.empty() ? nullptr : v.data();
        EXPECT_EQ(static_cast<long long>(sum_array(p, n)), oracle)
            << "n=" << n;
    }

    // Явные края: n == 0 всегда 0 (на любом указателе, даже nullptr).
    EXPECT_EQ(sum_array(nullptr, 0), 0);
    int one[] = {-7};
    EXPECT_EQ(sum_array(one, 1), -7);
}

// ── swap_ints: обмен значений и идемпотентность двойного обмена ───────────────
TEST(PointersProps, SwapIntsExchangesAndDoubleIsIdentity) {
    std::mt19937 rng(0x5EEDu);
    std::uniform_int_distribution<int> val(-100000, 100000);

    for (int iter = 0; iter < 400; ++iter) {
        int a0 = val(rng), b0 = val(rng);
        int a = a0, b = b0;

        swap_ints(&a, &b);
        EXPECT_EQ(a, b0);   // значения обменялись
        EXPECT_EQ(b, a0);

        swap_ints(&a, &b);  // второй обмен возвращает исходное
        EXPECT_EQ(a, a0);
        EXPECT_EQ(b, b0);
    }

    // Край: своп равных и своп переменной с самой собой (один адрес).
    int x = 42;
    swap_ints(&x, &x);
    EXPECT_EQ(x, 42);
}

// ── count_value: оракул против std::count; сумма по всем значениям == n ───────
TEST(PointersProps, CountValueMatchesStdCount) {
    std::mt19937 rng(0xBEEFu);
    std::uniform_int_distribution<int> sz(0, 40);
    // Маленький домен значений, чтобы реально были повторы и попадания.
    std::uniform_int_distribution<int> val(0, 5);

    for (int iter = 0; iter < 400; ++iter) {
        int n = sz(rng);
        std::vector<int> v(n);
        for (int& x : v) x = val(rng);
        const int* p = v.empty() ? nullptr : v.data();

        // Согласие с std::count для каждого значения домена.
        long long total = 0;
        for (int target = 0; target <= 5; ++target) {
            long long oracle = std::count(v.begin(), v.end(), target);
            EXPECT_EQ(count_value(p, n, target), static_cast<int>(oracle))
                << "n=" << n << " target=" << target;
            total += oracle;
        }
        // Инвариант: счётчики по всем значениям домена в сумме дают n
        // (домен [0,5] покрывает все возможные элементы).
        EXPECT_EQ(total, static_cast<long long>(n));

        // Отсутствующее значение даёт 0.
        EXPECT_EQ(count_value(p, n, 999), 0);
    }
}

// ── max_element_ptr: оракул против std::max_element; nullptr на пустом ────────
TEST(PointersProps, MaxElementPtrMatchesStd) {
    std::mt19937 rng(0xD00Du);
    std::uniform_int_distribution<int> sz(0, 50);
    std::uniform_int_distribution<int> val(-500, 500);

    for (int iter = 0; iter < 400; ++iter) {
        int n = sz(rng);
        std::vector<int> v(n);
        for (int& x : v) x = val(rng);
        const int* p = v.empty() ? nullptr : v.data();

        const int* got = max_element_ptr(p, n);
        if (n == 0) {
            EXPECT_EQ(got, nullptr);
            continue;
        }
        ASSERT_NE(got, nullptr) << "n=" << n;
        // Указатель должен лежать внутри буфера.
        ASSERT_GE(got, v.data());
        ASSERT_LT(got, v.data() + n);

        int oracle_max = *std::max_element(v.begin(), v.end());
        EXPECT_EQ(*got, oracle_max) << "n=" << n;
        // Это действительно максимум: никакой элемент не больше.
        for (int x : v) EXPECT_LE(x, *got);
    }
}

// ── reverse_in_place: инволюция, перестановка, оракул std::reverse ────────────
TEST(PointersProps, ReverseInPlaceIsInvolutionAndPermutation) {
    std::mt19937 rng(0x12345u);
    std::uniform_int_distribution<int> sz(0, 50);
    std::uniform_int_distribution<int> val(-1000, 1000);

    for (int iter = 0; iter < 400; ++iter) {
        int n = sz(rng);
        std::vector<int> orig(n);
        for (int& x : orig) x = val(rng);

        // Согласие с std::reverse.
        std::vector<int> a = orig;
        std::vector<int> expected = orig;
        std::reverse(expected.begin(), expected.end());
        reverse_in_place(a.empty() ? nullptr : a.data(), n);
        EXPECT_EQ(a, expected) << "n=" << n;

        // Это перестановка входа (мультимножество не меняется).
        std::vector<int> sa = a, so = orig;
        std::sort(sa.begin(), sa.end());
        std::sort(so.begin(), so.end());
        EXPECT_EQ(sa, so);

        // Инволюция: разворот разворота возвращает исходный массив.
        reverse_in_place(a.empty() ? nullptr : a.data(), n);
        EXPECT_EQ(a, orig);
    }
}

// ── reverse_with_pointers: оракул std::reverse и согласие с reverse_in_place ──
TEST(PointersProps, ReverseWithPointersMatchesStdAndIndexVersion) {
    std::mt19937 rng(0xABCDEu);
    std::uniform_int_distribution<int> sz(0, 50);
    std::uniform_int_distribution<int> val(-1000, 1000);

    for (int iter = 0; iter < 400; ++iter) {
        int n = sz(rng);
        std::vector<int> orig(n);
        for (int& x : orig) x = val(rng);

        std::vector<int> a = orig, b = orig;
        std::vector<int> expected = orig;
        std::reverse(expected.begin(), expected.end());

        reverse_with_pointers(a.empty() ? nullptr : a.data(), n);
        reverse_in_place(b.empty() ? nullptr : b.data(), n);

        // Версия на указателях == std::reverse == версия на индексах.
        EXPECT_EQ(a, expected) << "n=" << n;
        EXPECT_EQ(a, b);

        // Инволюция.
        reverse_with_pointers(a.empty() ? nullptr : a.data(), n);
        EXPECT_EQ(a, orig);
    }

    // Явные края: n == 0 (nullptr допустим) и n == 1 — без изменений.
    reverse_with_pointers(nullptr, 0);
    int one[] = {77};
    reverse_with_pointers(one, 1);
    EXPECT_EQ(one[0], 77);
}

// ── DynArray: конструктор-заполнение, sum == оракул, запись насквозь ──────────
TEST(DynArrayProps, ConstructFillSumAndWriteThrough) {
    std::mt19937 rng(0x44332211u);
    std::uniform_int_distribution<int> sz(0, 64);
    std::uniform_int_distribution<int> val(-200, 200);

    for (int iter = 0; iter < 300; ++iter) {
        int n = sz(rng);
        int fill = val(rng);

        DynArray a(n, fill);
        EXPECT_EQ(a.size(), n);

        // После конструктора все элементы == fill; sum == n*fill.
        long long expected_sum = static_cast<long long>(n) * fill;
        EXPECT_EQ(static_cast<long long>(a.sum()), expected_sum) << "n=" << n;
        for (int i = 0; i < n; ++i) EXPECT_EQ(a.at(i), fill);

        // Запись насквозь через неконстантную at(): зеркалим в vector-оракул.
        std::vector<int> mirror(n, fill);
        for (int i = 0; i < n; ++i) {
            int w = val(rng);
            a.at(i) = w;
            mirror[i] = w;
        }
        // sum совпадает со std::accumulate по зеркалу.
        long long oracle = std::accumulate(mirror.begin(), mirror.end(), 0LL);
        EXPECT_EQ(static_cast<long long>(a.sum()), oracle);
        for (int i = 0; i < n; ++i) EXPECT_EQ(a.at(i), mirror[i]);
    }
}

// ── DynArray::fill перезаписывает всё; границы at() бросают out_of_range ──────
TEST(DynArrayProps, FillOverwritesAndAtBoundsThrow) {
    std::mt19937 rng(0x99AA55u);
    std::uniform_int_distribution<int> sz(1, 40);   // n >= 1, чтобы было что заполнять
    std::uniform_int_distribution<int> val(-500, 500);

    for (int iter = 0; iter < 300; ++iter) {
        int n = sz(rng);
        DynArray a(n, val(rng));

        int v = val(rng);
        a.fill(v);
        EXPECT_EQ(static_cast<long long>(a.sum()),
                  static_cast<long long>(n) * v);
        for (int i = 0; i < n; ++i) EXPECT_EQ(a.at(i), v);

        // Любой валидный индекс [0, n) не бросает.
        std::uniform_int_distribution<int> idx(0, n - 1);
        EXPECT_NO_THROW(a.at(idx(rng)));

        // Индексы вне [0, n) бросают ровно std::out_of_range.
        EXPECT_THROW(a.at(-1), std::out_of_range);
        EXPECT_THROW(a.at(n), std::out_of_range);
        std::uniform_int_distribution<int> below(-10000, -1);
        std::uniform_int_distribution<int> above(n, n + 10000);
        EXPECT_THROW(a.at(below(rng)), std::out_of_range);
        EXPECT_THROW(a.at(above(rng)), std::out_of_range);
    }

    // Край: пустой массив — любой at() вне диапазона бросает.
    DynArray empty(0, 5);
    EXPECT_EQ(empty.size(), 0);
    EXPECT_THROW(empty.at(0), std::out_of_range);
    EXPECT_THROW(empty.at(-1), std::out_of_range);
}

// ── Задание 9: DynArray — правило трёх ───────────────────────────────────────────

// Краевые случаи: копирование пустого и единичного массивов.
TEST(DynArrayRuleOfThree, CopyEmptyArray) {
    DynArray a(0, 99);
    DynArray b = a;
    EXPECT_EQ(b.size(), 0);
    EXPECT_EQ(b.sum(), 0);
}

TEST(DynArrayRuleOfThree, CopyConstructorIsDeep) {
    DynArray a(4, 7);
    DynArray b = a;
    EXPECT_EQ(b.size(), 4);
    // Те же значения.
    for (int i = 0; i < b.size(); ++i) EXPECT_EQ(b.at(i), 7);
    // Изменение копии не затрагивает оригинал.
    b.at(0) = 999;
    EXPECT_EQ(a.at(0), 7);
    EXPECT_EQ(b.at(0), 999);
}

TEST(DynArrayRuleOfThree, CopyAssignmentIsDeep) {
    DynArray a(3, 5);
    DynArray b(3, 0);
    b = a;
    EXPECT_EQ(b.size(), 3);
    for (int i = 0; i < b.size(); ++i) EXPECT_EQ(b.at(i), 5);
    // Независимость после присваивания.
    a.at(1) = 42;
    EXPECT_EQ(b.at(1), 5);  // b не изменился
}

TEST(DynArrayRuleOfThree, SelfAssignmentSafe) {
    DynArray a(3, 11);
    // Самоприсваивание не должно ни крашиться, ни менять данные.
    DynArray& ref = a;
    ref = a;
    EXPECT_EQ(a.size(), 3);
    EXPECT_EQ(a.at(0), 11);
    EXPECT_EQ(a.at(1), 11);
    EXPECT_EQ(a.at(2), 11);
}

TEST(DynArrayRuleOfThree, CopyToDifferentSizeTarget) {
    // Присваивание массива большего размера в меньший (и наоборот).
    DynArray big(10, 1);
    DynArray small(2, 99);
    small = big;
    EXPECT_EQ(small.size(), 10);
    EXPECT_EQ(small.sum(), 10);

    DynArray big2(8, 2);
    DynArray small2(2, 3);
    big2 = small2;
    EXPECT_EQ(big2.size(), 2);
    EXPECT_EQ(big2.sum(), 6);
}

// ── Seeded property-тест: глубокая копия элементно согласована
//    и независима после модификации ────────────────────────────────────────────
TEST(DynArrayRuleOfThree, SeededPropertyDeepCopyMatchesAndIsIndependent) {
    // Фиксированный сид — тест детерминирован и не флакает.
    std::mt19937 rng(0xC0DE42u);
    std::uniform_int_distribution<int> sz(0, 40);
    std::uniform_int_distribution<int> val(-1000, 1000);

    for (int iter = 0; iter < 300; ++iter) {
        int n = sz(rng);
        // Строим DynArray со случайными значениями через at().
        DynArray a(n, 0);
        std::vector<int> expected(n);
        for (int i = 0; i < n; ++i) {
            expected[i] = val(rng);
            a.at(i) = expected[i];
        }

        // Инвариант 1: copy-constructor — поэлементное равенство.
        DynArray b = a;
        ASSERT_EQ(b.size(), n) << "iter=" << iter;
        for (int i = 0; i < n; ++i) {
            EXPECT_EQ(b.at(i), expected[i]) << "iter=" << iter << " i=" << i;
        }

        // Инвариант 2: изменение копии не меняет оригинал.
        if (n > 0) {
            b.at(0) = val(rng) + 100000;  // мутируем копию
            EXPECT_EQ(a.at(0), expected[0]) << "iter=" << iter;
        }

        // Инвариант 3: copy-assignment — поэлементное равенство.
        DynArray c(sz(rng), val(rng));  // c произвольного размера
        c = a;
        ASSERT_EQ(c.size(), n) << "iter=" << iter;
        for (int i = 0; i < n; ++i) {
            EXPECT_EQ(c.at(i), expected[i]) << "iter=" << iter << " i=" << i;
        }

        // Инвариант 4: изменение источника a после присваивания не трогает c.
        if (n > 0) {
            int saved = c.at(0);
            a.at(0) = val(rng) - 200000;  // мутируем источник
            EXPECT_EQ(c.at(0), saved) << "iter=" << iter;
        }
    }
}

// ── OwnedInt: глубокая копия — отдельный буфер и независимость значений ───────
TEST(OwnedIntProps, DeepCopyHasSeparateBufferAndIsIndependent) {
    std::mt19937 rng(0x0DDBEEFu);
    std::uniform_int_distribution<int> val(-100000, 100000);

    for (int iter = 0; iter < 300; ++iter) {
        int v0 = val(rng);
        OwnedInt a(v0);

        // Копия конструктором: то же значение, ДРУГОЙ буфер.
        OwnedInt b(a);
        EXPECT_EQ(b.get(), v0);
        EXPECT_NE(a.raw(), b.raw());

        // Меняем копию — оригинал не задет (буферы независимы).
        int v1 = val(rng);
        b.set(v1);
        EXPECT_EQ(b.get(), v1);
        EXPECT_EQ(a.get(), v0);

        // Присваивание: тоже глубокое и независимое.
        OwnedInt c(val(rng));
        c = a;
        EXPECT_EQ(c.get(), v0);
        EXPECT_NE(c.raw(), a.raw());
        int v2 = val(rng);
        a.set(v2);                 // меняем источник после присваивания
        EXPECT_EQ(c.get(), v0);    // у c — своё значение, не задето
        EXPECT_EQ(a.get(), v2);

        // Самоприсваивание не ломает значение и не меняет буфер.
        const int* before = c.raw();
        OwnedInt& cref = c;
        cref = c;
        EXPECT_EQ(c.get(), v0);
        EXPECT_EQ(c.raw(), before);
    }
}
