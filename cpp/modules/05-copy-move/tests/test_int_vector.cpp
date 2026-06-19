#include <gtest/gtest.h>
#include <utility>      // std::move
#include <stdexcept>    // std::out_of_range
#include <type_traits>  // std::is_copy_constructible и т.п.
#include <random>       // std::mt19937, распределения
#include <vector>       // std::vector как оракул
#include <cstddef>      // std::size_t
#include "int_vector.hpp"

TEST(IntVector, EmptyByDefault) {
    IntVector v;
    EXPECT_EQ(v.size(), 0u);
    EXPECT_TRUE(v.empty());
}

TEST(IntVector, FillConstructor) {
    IntVector v(3, 7);
    ASSERT_EQ(v.size(), 3u);
    EXPECT_EQ(v[0], 7);
    EXPECT_EQ(v[1], 7);
    EXPECT_EQ(v[2], 7);
    EXPECT_GE(v.capacity(), v.size());
}

TEST(IntVector, PushBackGrows) {
    IntVector v;
    for (int i = 0; i < 10; ++i) v.push_back(i * i);
    ASSERT_EQ(v.size(), 10u);
    EXPECT_GE(v.capacity(), 10u);
    EXPECT_EQ(v[0], 0);
    EXPECT_EQ(v[3], 9);
    EXPECT_EQ(v[9], 81);
}

TEST(IntVector, IndexIsWritable) {
    IntVector v(2, 0);
    v[0] = 42;
    EXPECT_EQ(v[0], 42);
}

TEST(IntVector, DeepCopyIndependent) {
    IntVector a(3, 1);
    IntVector b = a;        // копирующий конструктор
    b[0] = 99;
    EXPECT_EQ(a[0], 1);     // оригинал НЕ изменился => копия глубокая
    EXPECT_EQ(b[0], 99);
    EXPECT_EQ(a.size(), b.size());
}

TEST(IntVector, CopyAssignment) {
    IntVector a(2, 5);
    IntVector b;
    b = a;                  // копирующее присваивание
    ASSERT_EQ(b.size(), 2u);
    b[1] = 0;
    EXPECT_EQ(a[1], 5);
}

TEST(IntVector, SelfAssignmentSafe) {
    IntVector a(3, 4);
    a = a;                  // не должно ломаться/течь
    ASSERT_EQ(a.size(), 3u);
    EXPECT_EQ(a[2], 4);
}

TEST(IntVector, MoveConstructorStealsBuffer) {
    IntVector a(4, 8);
    IntVector b = std::move(a);  // перемещающий конструктор
    EXPECT_EQ(b.size(), 4u);
    EXPECT_EQ(b[0], 8);
    EXPECT_EQ(a.size(), 0u);     // источник стал пустым
}

TEST(IntVector, MoveAssignment) {
    IntVector a(4, 2);
    IntVector b;
    b = std::move(a);
    EXPECT_EQ(b.size(), 4u);
    EXPECT_EQ(a.size(), 0u);
}

// ───────────────────────────────────────────────────────────────────────────
// Задание 2: Buffer
// ───────────────────────────────────────────────────────────────────────────

TEST(Buffer, FillConstructor) {
    Buffer b(4, 2.5);
    ASSERT_EQ(b.size(), 4u);
    ASSERT_NE(b.data(), nullptr);
    EXPECT_DOUBLE_EQ(b[0], 2.5);
    EXPECT_DOUBLE_EQ(b[3], 2.5);
}

TEST(Buffer, DefaultValueIsZero) {
    Buffer b(3);
    ASSERT_EQ(b.size(), 3u);
    EXPECT_DOUBLE_EQ(b[0], 0.0);
    EXPECT_DOUBLE_EQ(b[1], 0.0);
    EXPECT_DOUBLE_EQ(b[2], 0.0);
}

TEST(Buffer, EmptyBufferHasNullData) {
    Buffer empty(0);
    EXPECT_EQ(empty.size(), 0u);
    EXPECT_EQ(empty.data(), nullptr);
    // а вот НЕпустой буфер обязан иметь реальную память — это отличает
    // настоящую реализацию от заглушки, всегда отдающей nullptr
    Buffer nonempty(4);
    EXPECT_EQ(nonempty.size(), 4u);
    EXPECT_NE(nonempty.data(), nullptr);
}

TEST(Buffer, IndexIsWritable) {
    Buffer b(2, 0.0);
    b[1] = 9.0;
    EXPECT_DOUBLE_EQ(b[1], 9.0);
    EXPECT_DOUBLE_EQ(b[0], 0.0);
}

TEST(Buffer, FillOverwritesAll) {
    Buffer b(3, 1.0);
    b.fill(-4.0);
    EXPECT_DOUBLE_EQ(b[0], -4.0);
    EXPECT_DOUBLE_EQ(b[1], -4.0);
    EXPECT_DOUBLE_EQ(b[2], -4.0);
}

TEST(Buffer, AtChecksBounds) {
    Buffer b(2, 1.0);
    EXPECT_DOUBLE_EQ(b.at(0), 1.0);
    EXPECT_DOUBLE_EQ(b.at(1), 1.0);
    EXPECT_THROW(b.at(2), std::out_of_range);   // ровно std::out_of_range
    EXPECT_THROW(b.at(999), std::out_of_range);
}

TEST(Buffer, CopyIsDeepAndIndependent) {
    Buffer a(3, 7.0);
    Buffer b = a;                 // копирующий конструктор
    ASSERT_EQ(b.size(), 3u);
    ASSERT_NE(b.data(), a.data()); // РАЗНЫЕ буферы
    b[0] = 100.0;
    EXPECT_DOUBLE_EQ(a[0], 7.0);   // оригинал не тронут
    EXPECT_DOUBLE_EQ(b[0], 100.0);
}

TEST(Buffer, CopyAssignmentIsDeep) {
    Buffer a(2, 5.0);
    Buffer b(5, 0.0);
    b = a;                         // копирующее присваивание (разные размеры!)
    ASSERT_EQ(b.size(), 2u);
    ASSERT_NE(b.data(), a.data());
    b[0] = -1.0;
    EXPECT_DOUBLE_EQ(a[0], 5.0);
}

TEST(Buffer, SelfAssignmentSafe) {
    Buffer a(3, 4.0);
    Buffer& ref = a;
    a = ref;                       // самоприсваивание не должно ломать/чистить данные
    ASSERT_EQ(a.size(), 3u);
    EXPECT_DOUBLE_EQ(a[2], 4.0);
}

TEST(Buffer, MoveConstructorStealsBuffer) {
    Buffer a(4, 8.0);
    const double* old_ptr = a.data();
    Buffer b = std::move(a);       // перемещающий конструктор
    EXPECT_EQ(b.size(), 4u);
    EXPECT_EQ(b.data(), old_ptr);  // тот же буфер «переехал» без копирования
    EXPECT_DOUBLE_EQ(b[0], 8.0);
    EXPECT_EQ(a.size(), 0u);       // источник опустошён
    EXPECT_EQ(a.data(), nullptr);
}

TEST(Buffer, MoveAssignmentStealsBuffer) {
    Buffer a(4, 2.0);
    const double* old_ptr = a.data();
    Buffer b(1, 0.0);
    b = std::move(a);
    EXPECT_EQ(b.size(), 4u);
    EXPECT_EQ(b.data(), old_ptr);
    EXPECT_EQ(a.size(), 0u);
    EXPECT_EQ(a.data(), nullptr);
}

TEST(Buffer, SwapExchangesContents) {
    Buffer a(2, 1.0);
    Buffer b(3, 9.0);
    a.swap(b);
    EXPECT_EQ(a.size(), 3u);
    EXPECT_EQ(b.size(), 2u);
    EXPECT_DOUBLE_EQ(a[0], 9.0);
    EXPECT_DOUBLE_EQ(b[0], 1.0);
}

TEST(Buffer, MoveIsNoexcept) {
    // правильно написанный move обязан быть noexcept — это требуют контейнеры
    EXPECT_TRUE(std::is_nothrow_move_constructible_v<Buffer>);
    EXPECT_TRUE(std::is_nothrow_move_assignable_v<Buffer>);
}

// ───────────────────────────────────────────────────────────────────────────
// Задание 3: UniqueHandle
// ───────────────────────────────────────────────────────────────────────────

// move-only тип на этапе КОМПИЛЯЦИИ: копировать нельзя, перемещать можно
static_assert(!std::is_copy_constructible_v<UniqueHandle>, "UniqueHandle must NOT be copyable");
static_assert(!std::is_copy_assignable_v<UniqueHandle>,    "UniqueHandle must NOT be copy-assignable");
static_assert(std::is_move_constructible_v<UniqueHandle>,  "UniqueHandle must be movable");
static_assert(std::is_move_assignable_v<UniqueHandle>,     "UniqueHandle must be move-assignable");

TEST(UniqueHandle, DefaultIsEmpty) {
    resource_reset_registry();
    UniqueHandle h;
    EXPECT_FALSE(h.valid());
    EXPECT_EQ(h.get(), kInvalidHandle);
    EXPECT_FALSE(static_cast<bool>(h));
    EXPECT_EQ(resource_live_count(), 0);
}

TEST(UniqueHandle, MakeHandleAcquires) {
    resource_reset_registry();
    {
        UniqueHandle h = make_handle();
        EXPECT_TRUE(h.valid());
        EXPECT_NE(h.get(), kInvalidHandle);
        EXPECT_EQ(resource_live_count(), 1);   // ресурс захвачен
    }
    EXPECT_EQ(resource_live_count(), 0);        // деструктор освободил его
}

TEST(UniqueHandle, DestructorReleases) {
    resource_reset_registry();
    {
        UniqueHandle h(resource_acquire());
        EXPECT_EQ(resource_live_count(), 1);
    }
    EXPECT_EQ(resource_live_count(), 0);
}

TEST(UniqueHandle, MoveTransfersOwnership) {
    resource_reset_registry();
    UniqueHandle a = make_handle();
    int id = a.get();
    UniqueHandle b = std::move(a);
    EXPECT_EQ(b.get(), id);
    EXPECT_TRUE(b.valid());
    EXPECT_FALSE(a.valid());                    // источник опустошён
    EXPECT_EQ(a.get(), kInvalidHandle);
    EXPECT_EQ(resource_live_count(), 1);        // владелец один — ресурс не задвоился
}

TEST(UniqueHandle, MoveAssignReleasesOldThenSteals) {
    resource_reset_registry();
    UniqueHandle a = make_handle();   // id A
    UniqueHandle b = make_handle();   // id B
    EXPECT_EQ(resource_live_count(), 2);
    int idA = a.get();
    b = std::move(a);                 // b должен освободить B, забрать A
    EXPECT_EQ(b.get(), idA);
    EXPECT_FALSE(a.valid());
    EXPECT_EQ(resource_live_count(), 1);  // B освобождён, A жив => ровно 1
}

TEST(UniqueHandle, ReleaseHandsOutOwnership) {
    resource_reset_registry();
    UniqueHandle h = make_handle();
    int id = h.release();             // отдать наружу, НЕ освобождая
    EXPECT_NE(id, kInvalidHandle);
    EXPECT_FALSE(h.valid());
    EXPECT_EQ(resource_live_count(), 1);  // ресурс ещё жив — теперь за него отвечаем мы
    resource_release(id);                  // освобождаем вручную
    EXPECT_EQ(resource_live_count(), 0);
}

TEST(UniqueHandle, ResetReplacesResource) {
    resource_reset_registry();
    UniqueHandle h = make_handle();
    int first = h.get();
    h.reset(resource_acquire());     // освободить первый, взять второй
    EXPECT_NE(h.get(), first);
    EXPECT_TRUE(h.valid());
    EXPECT_EQ(resource_live_count(), 1);  // первый освобождён, второй жив

    h.reset();                       // сбросить в пустоту
    EXPECT_FALSE(h.valid());
    EXPECT_EQ(resource_live_count(), 0);
}

TEST(UniqueHandle, SwapExchangesIds) {
    resource_reset_registry();
    UniqueHandle a = make_handle();
    UniqueHandle b;
    int id = a.get();
    a.swap(b);
    EXPECT_FALSE(a.valid());
    EXPECT_TRUE(b.valid());
    EXPECT_EQ(b.get(), id);
    EXPECT_EQ(resource_live_count(), 1);
}

TEST(UniqueHandle, NoLeakAcrossManyMoves) {
    resource_reset_registry();
    {
        UniqueHandle h = make_handle();
        for (int i = 0; i < 5; ++i) {
            UniqueHandle tmp = std::move(h);
            h = std::move(tmp);       // гоняем владение туда-сюда
        }
        EXPECT_TRUE(h.valid());
        EXPECT_EQ(resource_live_count(), 1);
    }
    EXPECT_EQ(resource_live_count(), 0);
}

// ===== РАНДОМИЗИРОВАННЫЕ / PROPERTY-ТЕСТЫ (детерминированы фиксированным сидом) =====
//
// Идея: вместо фиксированных примеров прогоняем СОТНИ случайных входов и
// проверяем ИНВАРИАНТЫ, которые обязаны держаться у любой корректной
// реализации правила пяти:
//   * push_back ведёт себя как std::vector (оракул);
//   * копия — глубокая и независимая, размер и содержимое совпадают;
//   * move «крадёт» буфер: цель == бывшее содержимое, источник пуст;
//   * swap — это перестановка содержимого (две полусы swap == тождество);
//   * copy-assign переживает разные размеры и самоприсваивание;
//   * у UniqueHandle реестр всегда сбалансирован (нет утечек / двойных release).
// Сид фиксирован (0xC0FFEE) => в CI ничего не «мигает».

namespace {

// Снимок содержимого вектора для сравнения «до/после».
std::vector<int> snapshot(const IntVector& v) {
    std::vector<int> out;
    out.reserve(v.size());
    for (std::size_t i = 0; i < v.size(); ++i) out.push_back(v[i]);
    return out;
}

std::vector<double> snapshot(const Buffer& b) {
    std::vector<double> out;
    out.reserve(b.size());
    for (std::size_t i = 0; i < b.size(); ++i) out.push_back(b[i]);
    return out;
}

}  // namespace

// --- IntVector ---------------------------------------------------------------

// push_back в IntVector ведёт себя ровно как в std::vector<int> (оракул).
TEST(IntVectorProps, PushBackMatchesStdVectorOracle) {
    std::mt19937 rng(0xC0FFEEu);
    std::uniform_int_distribution<int> val(-100000, 100000);
    std::uniform_int_distribution<int> len(0, 64);

    for (int iter = 0; iter < 300; ++iter) {
        const int n = len(rng);
        IntVector v;
        std::vector<int> oracle;
        for (int i = 0; i < n; ++i) {
            int x = val(rng);
            v.push_back(x);
            oracle.push_back(x);
        }
        ASSERT_EQ(v.size(), oracle.size());
        EXPECT_EQ(v.empty(), oracle.empty());
        // ёмкость никогда не меньше размера
        EXPECT_GE(v.capacity(), v.size());
        for (std::size_t i = 0; i < oracle.size(); ++i) {
            ASSERT_EQ(v[i], oracle[i]) << "iter=" << iter << " i=" << i;
        }
    }
}

// Копия независима: правка копии не трогает оригинал, размеры/содержимое равны.
TEST(IntVectorProps, CopyIsDeepAndIndependent) {
    std::mt19937 rng(0xC0FFEEu);
    std::uniform_int_distribution<int> val(-50000, 50000);
    std::uniform_int_distribution<int> len(0, 50);

    for (int iter = 0; iter < 300; ++iter) {
        const int n = len(rng);
        IntVector a;
        for (int i = 0; i < n; ++i) a.push_back(val(rng));
        const std::vector<int> before = snapshot(a);

        IntVector b = a;                       // копирующий конструктор
        ASSERT_EQ(b.size(), a.size());
        EXPECT_EQ(snapshot(b), before);

        // мутируем копию целиком — оригинал обязан остаться прежним
        for (std::size_t i = 0; i < b.size(); ++i) b[i] = b[i] + 12345;
        EXPECT_EQ(snapshot(a), before) << "iter=" << iter << ": copy was shallow";
    }
}

// Move-конструктор: цель получает прежнее содержимое, источник становится пустым.
TEST(IntVectorProps, MoveConstructorStealsContent) {
    std::mt19937 rng(0xC0FFEEu);
    std::uniform_int_distribution<int> val(-50000, 50000);
    std::uniform_int_distribution<int> len(0, 50);

    for (int iter = 0; iter < 300; ++iter) {
        const int n = len(rng);
        IntVector a;
        for (int i = 0; i < n; ++i) a.push_back(val(rng));
        const std::vector<int> before = snapshot(a);

        IntVector b = std::move(a);            // перемещающий конструктор
        EXPECT_EQ(snapshot(b), before);
        EXPECT_EQ(a.size(), 0u) << "iter=" << iter << ": source not emptied";
        EXPECT_TRUE(a.empty());
    }
}

// Move-присваивание тоже крадёт содержимое и опустошает источник.
TEST(IntVectorProps, MoveAssignmentStealsContent) {
    std::mt19937 rng(0xC0FFEEu);
    std::uniform_int_distribution<int> val(-50000, 50000);
    std::uniform_int_distribution<int> len(0, 50);

    for (int iter = 0; iter < 300; ++iter) {
        IntVector a;
        for (int i = 0, n = len(rng); i < n; ++i) a.push_back(val(rng));
        const std::vector<int> before = snapshot(a);

        IntVector b;
        for (int i = 0, n = len(rng); i < n; ++i) b.push_back(val(rng));  // непустая цель
        b = std::move(a);
        EXPECT_EQ(snapshot(b), before);
        EXPECT_EQ(a.size(), 0u);
    }
}

// Copy-assign между векторами разных размеров + самоприсваивание не портят данные.
TEST(IntVectorProps, CopyAssignmentSurvivesAnySizesAndSelf) {
    std::mt19937 rng(0xC0FFEEu);
    std::uniform_int_distribution<int> val(-50000, 50000);
    std::uniform_int_distribution<int> len(0, 40);

    for (int iter = 0; iter < 300; ++iter) {
        IntVector a;
        for (int i = 0, n = len(rng); i < n; ++i) a.push_back(val(rng));
        IntVector b;
        for (int i = 0, n = len(rng); i < n; ++i) b.push_back(val(rng));

        const std::vector<int> a_snap = snapshot(a);
        b = a;                                  // присваивание (размеры произвольны)
        EXPECT_EQ(snapshot(b), a_snap);
        EXPECT_EQ(snapshot(a), a_snap);         // источник не тронут

        IntVector& self = a;
        a = self;                               // самоприсваивание
        EXPECT_EQ(snapshot(a), a_snap) << "iter=" << iter << ": self-assign corrupted data";
    }
}

// --- Buffer ------------------------------------------------------------------

// Конструктор Buffer(n, v): размер == n, все элементы == v, пустой => nullptr.
TEST(BufferProps, ConstructorFillsAndSizes) {
    std::mt19937 rng(0xC0FFEEu);
    std::uniform_int_distribution<int> len(0, 64);
    std::uniform_real_distribution<double> val(-1000.0, 1000.0);

    for (int iter = 0; iter < 300; ++iter) {
        const std::size_t n = static_cast<std::size_t>(len(rng));
        const double v = val(rng);
        Buffer b(n, v);
        ASSERT_EQ(b.size(), n);
        if (n == 0) {
            EXPECT_EQ(b.data(), nullptr);
        } else {
            EXPECT_NE(b.data(), nullptr);
            for (std::size_t i = 0; i < n; ++i) EXPECT_DOUBLE_EQ(b[i], v);
        }
    }
}

// Копия Buffer глубокая: иной указатель, равное содержимое, независимость.
TEST(BufferProps, CopyIsDeepAndIndependent) {
    std::mt19937 rng(0xC0FFEEu);
    std::uniform_int_distribution<int> len(0, 50);
    std::uniform_real_distribution<double> val(-1000.0, 1000.0);

    for (int iter = 0; iter < 300; ++iter) {
        const std::size_t n = static_cast<std::size_t>(len(rng));
        Buffer a(n, 0.0);
        for (std::size_t i = 0; i < n; ++i) a[i] = val(rng);
        const std::vector<double> before = snapshot(a);

        Buffer b = a;
        ASSERT_EQ(b.size(), a.size());
        EXPECT_EQ(snapshot(b), before);
        if (n > 0) EXPECT_NE(b.data(), a.data());   // разные буферы

        for (std::size_t i = 0; i < b.size(); ++i) b[i] = b[i] + 777.0;
        EXPECT_EQ(snapshot(a), before) << "iter=" << iter << ": copy was shallow";
    }
}

// Move-конструктор Buffer: тот же указатель «переезжает», источник -> (nullptr, 0).
TEST(BufferProps, MoveConstructorTransfersPointer) {
    std::mt19937 rng(0xC0FFEEu);
    std::uniform_int_distribution<int> len(1, 50);   // непустой, чтобы был реальный буфер
    std::uniform_real_distribution<double> val(-1000.0, 1000.0);

    for (int iter = 0; iter < 300; ++iter) {
        const std::size_t n = static_cast<std::size_t>(len(rng));
        Buffer a(n, 0.0);
        for (std::size_t i = 0; i < n; ++i) a[i] = val(rng);
        const std::vector<double> before = snapshot(a);
        const double* old_ptr = a.data();

        Buffer b = std::move(a);
        EXPECT_EQ(b.size(), n);
        EXPECT_EQ(b.data(), old_ptr);          // буфер переехал без копии
        EXPECT_EQ(snapshot(b), before);
        EXPECT_EQ(a.size(), 0u);
        EXPECT_EQ(a.data(), nullptr);
    }
}

// swap — это перестановка: применённый дважды, он возвращает исходное состояние.
TEST(BufferProps, SwapIsAnInvolutionPermutation) {
    std::mt19937 rng(0xC0FFEEu);
    std::uniform_int_distribution<int> len(0, 40);
    std::uniform_real_distribution<double> val(-1000.0, 1000.0);

    for (int iter = 0; iter < 300; ++iter) {
        Buffer a(static_cast<std::size_t>(len(rng)), 0.0);
        for (std::size_t i = 0; i < a.size(); ++i) a[i] = val(rng);
        Buffer b(static_cast<std::size_t>(len(rng)), 0.0);
        for (std::size_t i = 0; i < b.size(); ++i) b[i] = val(rng);

        const std::vector<double> a0 = snapshot(a);
        const std::vector<double> b0 = snapshot(b);

        a.swap(b);
        EXPECT_EQ(snapshot(a), b0);            // содержимое обменялось
        EXPECT_EQ(snapshot(b), a0);
        EXPECT_EQ(a.size(), b0.size());
        EXPECT_EQ(b.size(), a0.size());

        a.swap(b);                             // второй swap — тождество
        EXPECT_EQ(snapshot(a), a0) << "iter=" << iter << ": swap is not an involution";
        EXPECT_EQ(snapshot(b), b0);
    }
}

// at() бросает ровно std::out_of_range за границей и не бросает внутри.
TEST(BufferProps, AtThrowsOutOfRangeBeyondBounds) {
    std::mt19937 rng(0xC0FFEEu);
    std::uniform_int_distribution<int> len(0, 32);

    for (int iter = 0; iter < 300; ++iter) {
        const std::size_t n = static_cast<std::size_t>(len(rng));
        Buffer b(n, 1.5);
        for (std::size_t i = 0; i < n; ++i) {
            EXPECT_NO_THROW({ volatile double d = b.at(i); (void)d; });
        }
        // n и n+произвольный сдвиг — заведомо вне границ
        std::uniform_int_distribution<int> over(0, 1000);
        EXPECT_THROW(b.at(n), std::out_of_range);
        EXPECT_THROW(b.at(n + static_cast<std::size_t>(over(rng))), std::out_of_range);
    }
}

// copy-assign Buffer переживает любые размеры цели и самоприсваивание.
TEST(BufferProps, CopyAssignmentSurvivesAnySizesAndSelf) {
    std::mt19937 rng(0xC0FFEEu);
    std::uniform_int_distribution<int> len(0, 40);
    std::uniform_real_distribution<double> val(-1000.0, 1000.0);

    for (int iter = 0; iter < 300; ++iter) {
        Buffer a(static_cast<std::size_t>(len(rng)), 0.0);
        for (std::size_t i = 0; i < a.size(); ++i) a[i] = val(rng);
        Buffer b(static_cast<std::size_t>(len(rng)), 0.0);
        for (std::size_t i = 0; i < b.size(); ++i) b[i] = val(rng);

        const std::vector<double> a_snap = snapshot(a);
        b = a;
        EXPECT_EQ(snapshot(b), a_snap);
        EXPECT_EQ(snapshot(a), a_snap);

        Buffer& self = a;
        a = self;                               // самоприсваивание не должно чистить данные
        EXPECT_EQ(snapshot(a), a_snap) << "iter=" << iter << ": self-assign corrupted data";
    }
}

// --- UniqueHandle ------------------------------------------------------------

// После любой случайной последовательности move/swap/reset в блоке реестр
// возвращается к нулю «живых» ресурсов: нет утечек и нет двойных release.
TEST(UniqueHandleProps, RegistryAlwaysBalancedAfterRandomOps) {
    std::mt19937 rng(0xC0FFEEu);
    std::uniform_int_distribution<int> op(0, 4);
    std::uniform_int_distribution<int> pick(0, 3);

    for (int iter = 0; iter < 300; ++iter) {
        resource_reset_registry();
        {
            UniqueHandle h[4];                  // четыре пустых хэндла
            const int steps = 30;
            for (int s = 0; s < steps; ++s) {
                const int i = pick(rng);
                const int j = pick(rng);
                switch (op(rng)) {
                    case 0: h[i] = make_handle();            break;  // захватить
                    case 1: if (i != j) h[i] = std::move(h[j]); break;  // move-assign
                    case 2: h[i].swap(h[j]);                 break;  // swap
                    case 3: h[i].reset();                    break;  // освободить
                    case 4: {                                        // release вручную
                        int id = h[i].release();
                        if (id != kInvalidHandle) resource_release(id);
                        break;
                    }
                }
                // инвариант: живых ресурсов не больше, чем валидных хэндлов
                int valid_cnt = 0;
                for (int k = 0; k < 4; ++k) if (h[k].valid()) ++valid_cnt;
                EXPECT_EQ(resource_live_count(), valid_cnt)
                    << "iter=" << iter << " step=" << s;
            }
        }
        // все хэндлы разрушены => ноль живых ресурсов (нет утечек)
        EXPECT_EQ(resource_live_count(), 0) << "iter=" << iter << ": leak after scope";
    }
}

// Move/swap сохраняют id (ресурс не подменяется и не задваивается).
TEST(UniqueHandleProps, MoveAndSwapPreserveIds) {
    std::mt19937 rng(0xC0FFEEu);

    for (int iter = 0; iter < 300; ++iter) {
        resource_reset_registry();
        UniqueHandle a = make_handle();
        const int idA = a.get();
        EXPECT_NE(idA, kInvalidHandle);

        UniqueHandle b = std::move(a);          // id переезжает целиком
        EXPECT_EQ(b.get(), idA);
        EXPECT_FALSE(a.valid());
        EXPECT_EQ(resource_live_count(), 1);    // ресурс не задвоился

        UniqueHandle c;
        b.swap(c);                              // swap с пустым перемещает id
        EXPECT_FALSE(b.valid());
        EXPECT_EQ(c.get(), idA);
        EXPECT_EQ(resource_live_count(), 1);

        // случайная серия туда-сюда move — id и баланс сохраняются
        std::uniform_int_distribution<int> n(0, 10);
        UniqueHandle cur = std::move(c);
        for (int k = 0, m = n(rng); k < m; ++k) {
            UniqueHandle tmp = std::move(cur);
            cur = std::move(tmp);
        }
        EXPECT_EQ(cur.get(), idA);
        EXPECT_EQ(resource_live_count(), 1);
    }
}
