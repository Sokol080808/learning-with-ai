#pragma once
//
// Модуль 21 — Реализуем контейнеры STL.
// Header-only: весь код шаблонов живёт здесь. Реализуй тела методов
// прямо в этом файле, заменяя `throw std::logic_error("TODO: ...")`
// и неправильные заглушки на корректное поведение.
//
#include <cstddef>      // std::size_t, std::ptrdiff_t
#include <stdexcept>    // std::out_of_range, std::logic_error
#include <utility>      // std::move, std::swap, std::pair
#include <new>          // placement new, ::operator new/delete
#include <iterator>     // std::random_access_iterator_tag и т.п.
#include <type_traits>  // std::aligned_storage_t (не обязателен)
#include <unordered_map>
#include <list>

// =====================================================================
// Задание 1. Vector<T> — динамический массив (упрощённый std::vector).
//
// Контракт:
//   - size()      — число хранимых элементов;
//   - capacity()  — сколько элементов помещается без перевыделения;
//   - push_back(x)— добавляет копию x в конец; при size()==capacity()
//                   ёмкость растёт (политика: 0 -> 1, иначе ×2);
//   - pop_back()  — убирает последний элемент (UB, если пусто — не
//                   проверяем, как и std::vector);
//   - operator[]  — доступ без проверки границ;
//   - at(i)       — доступ С проверкой: при i >= size() бросает
//                   std::out_of_range;
//   - begin()/end()— указатели на первый элемент и за последний
//                   (T* и const T*); по ним работает range-based for.
//
// Гарантии: после push_back, вызвавшего рост, capacity() строго больше
// прежней; size() увеличивается ровно на 1.
// =====================================================================
template <class T>
class Vector {
public:
    using value_type      = T;
    using size_type       = std::size_t;
    using iterator        = T*;
    using const_iterator  = const T*;

    Vector() = default;

    ~Vector() {
        // Корректная заглушка: ничего не освобождаем — учебная цель в том,
        // чтобы ты сам управлял временем жизни. Реализуешь — допиши деструктор.
    }

    // Запрещаем копирование/перемещение по умолчанию, пока ты не реализуешь
    // их сам (иначе компилятор сгенерировал бы поверхностные версии,
    // которые двойным free сломали бы ручное управление памятью).
    Vector(const Vector&)            = delete;
    Vector& operator=(const Vector&) = delete;

    void push_back(const T& value) {
        (void)value;
        throw std::logic_error("TODO: Vector::push_back");
    }

    void pop_back() {
        throw std::logic_error("TODO: Vector::pop_back");
    }

    T& operator[](size_type i) {
        // Неправильная заглушка: возвращает ссылку на статический мусор.
        (void)i;
        static T dummy{};
        return dummy;
    }
    const T& operator[](size_type i) const {
        (void)i;
        static T dummy{};
        return dummy;
    }

    T& at(size_type i) {
        (void)i;
        throw std::logic_error("TODO: Vector::at");
    }
    const T& at(size_type i) const {
        (void)i;
        throw std::logic_error("TODO: Vector::at");
    }

    size_type size() const {
        return 0;  // неправильная заглушка
    }
    size_type capacity() const {
        return 0;  // неправильная заглушка
    }
    bool empty() const {
        return true;  // неправильная заглушка
    }

    iterator       begin()        { return data_; }
    iterator       end()          { return data_; }   // неправильная заглушка
    const_iterator begin() const  { return data_; }
    const_iterator end()   const  { return data_; }   // неправильная заглушка

private:
    T*        data_ = nullptr;
    size_type size_ = 0;
    size_type cap_  = 0;
};

// =====================================================================
// Задание 2. SmallVector<T, N> — small-buffer optimization.
//
// Первые N элементов хранятся во встроенном буфере (на стеке, внутри
// объекта). Как только нужен (N+1)-й элемент, данные переезжают в кучу,
// и дальше контейнер ведёт себя как обычный динамический массив.
//
// Контракт:
//   - push_back/pop_back/size/operator[] — как у Vector;
//   - on_heap() — true тогда и только тогда, когда данные сейчас в куче
//                 (т.е. ёмкости встроенного буфера уже не хватило).
//   Пока size() <= N, on_heap() == false и capacity() == N.
// =====================================================================
template <class T, std::size_t N>
class SmallVector {
public:
    using size_type = std::size_t;

    SmallVector() = default;
    ~SmallVector() {
        // см. Vector::~Vector — реализуешь сам.
    }

    SmallVector(const SmallVector&)            = delete;
    SmallVector& operator=(const SmallVector&) = delete;

    void push_back(const T& value) {
        (void)value;
        throw std::logic_error("TODO: SmallVector::push_back");
    }
    void pop_back() {
        throw std::logic_error("TODO: SmallVector::pop_back");
    }

    T& operator[](size_type i) {
        (void)i;
        static T dummy{};
        return dummy;  // неправильная заглушка
    }
    const T& operator[](size_type i) const {
        (void)i;
        static T dummy{};
        return dummy;
    }

    size_type size() const     { return 0; }      // неправильная заглушка
    size_type capacity() const { return 0; }      // неправильная заглушка
    bool on_heap() const       { return true; }   // неправильная заглушка

private:
    // Подсказка по полям (можешь поменять под свою реализацию):
    //   T            inline_[N];   // встроенный буфер
    //   T*           heap_ = nullptr;
    //   size_type    size_ = 0;
    //   size_type    cap_  = N;
    T         inline_[N] {};
    T*        heap_ = nullptr;
    size_type size_ = 0;
    size_type cap_  = N;
};

// =====================================================================
// Задание 3. IntrusiveList<T> — интрузивный двусвязный список.
//
// «Интрузивный» = указатели next/prev живут ВНУТРИ самого узла, а не в
// отдельной обёртке. Пользователь создаёт Node<T> сам и отдаёт его
// списку; список не выделяет память под узлы.
//
// Контракт:
//   - push_back(node) — присоединяет узел в конец (node должен жить
//                       не меньше, чем находится в списке);
//   - erase(node)     — выдёргивает узел из списка за O(1), не удаляя
//                       сам объект Node;
//   - size()          — число узлов;
//   - front()/back()  — ссылка на значение первого/последнего узла.
// =====================================================================
template <class T>
struct Node {
    T     value;
    Node* next = nullptr;
    Node* prev = nullptr;
    explicit Node(const T& v) : value(v) {}
};

template <class T>
class IntrusiveList {
public:
    using size_type = std::size_t;

    IntrusiveList() = default;
    IntrusiveList(const IntrusiveList&)            = delete;
    IntrusiveList& operator=(const IntrusiveList&) = delete;

    void push_back(Node<T>* node) {
        (void)node;
        throw std::logic_error("TODO: IntrusiveList::push_back");
    }

    void erase(Node<T>* node) {
        (void)node;
        throw std::logic_error("TODO: IntrusiveList::erase");
    }

    size_type size() const { return 0; }  // неправильная заглушка
    bool empty() const     { return true; }

    T& front() {
        throw std::logic_error("TODO: IntrusiveList::front");
    }
    T& back() {
        throw std::logic_error("TODO: IntrusiveList::back");
    }

private:
    Node<T>*  head_ = nullptr;
    Node<T>*  tail_ = nullptr;
    size_type size_ = 0;
};

// =====================================================================
// Задание 4. LRUCache<K, V> — кэш с вытеснением Least Recently Used.
//
// Все операции — амортизированно O(1). Идея: std::list хранит пары
// (key, value) в порядке «давности использования» (front = самый
// свежий, back = кандидат на вытеснение), а unordered_map отображает
// ключ в итератор этого списка, чтобы за O(1) находить и перемещать узел.
//
// Контракт:
//   - LRUCache(capacity) — максимум элементов; capacity == 0 запрещён
//                          (бросай std::out_of_range);
//   - put(k, v) — вставляет/обновляет; если переполнение, вытесняет
//                 самый давно использованный элемент; и put, и get
//                 считаются «использованием» и двигают ключ во front;
//   - get(k)    — std::optional<V>: значение, если есть (и ключ
//                 становится свежим), иначе std::nullopt;
//   - contains(k), size().
// =====================================================================
template <class K, class V>
class LRUCache {
public:
    using size_type = std::size_t;

    explicit LRUCache(size_type capacity) {
        (void)capacity;
        throw std::logic_error("TODO: LRUCache(capacity)");
    }

    void put(const K& key, const V& value) {
        (void)key; (void)value;
        throw std::logic_error("TODO: LRUCache::put");
    }

    // Если ключа нет — std::nullopt.
    // (Возвращаем по значению, чтобы не зависеть от <optional> в сигнатуре
    //  заглушки; см. определение get ниже.)
    bool contains(const K& key) const {
        (void)key;
        return false;  // неправильная заглушка
    }

    size_type size() const     { return 0; }  // неправильная заглушка
    size_type capacity() const { return 0; }  // неправильная заглушка

    // get объявлен ниже как шаблонный метод, чтобы вернуть std::optional<V>.
    // Реализацию пиши там же.
    template <class Self = LRUCache>
    auto get(const K& key);

private:
    using ListType = std::list<std::pair<K, V>>;
    size_type capacity_ = 0;
    ListType  order_;                                   // front = свежий
    std::unordered_map<K, typename ListType::iterator> index_;
};

// =====================================================================
// Задание 5. RingBuffer<T, N> — кольцевой буфер фиксированной ёмкости.
//
// Хранит до N элементов в массиве, переиспользуя ячейки по кругу.
// Два индекса (head — откуда читать, tail — куда писать) и счётчик size.
//
// Контракт:
//   - try_push(x) — кладёт x в конец; если буфер полон, НЕ пишет и
//                   возвращает false; иначе true;
//   - push(x)     — то же, но при переполнении бросает std::out_of_range;
//   - try_pop()   — std::optional<T>: снимает и возвращает первый
//                   элемент, либо std::nullopt, если пусто;
//   - size(), full(), empty(), capacity() == N.
//
// FIFO: что положили первым, то и снимется первым.
// =====================================================================
template <class T, std::size_t N>
class RingBuffer {
    static_assert(N > 0, "RingBuffer requires N > 0");
public:
    using size_type = std::size_t;

    RingBuffer() = default;

    bool try_push(const T& value) {
        (void)value;
        return false;  // неправильная заглушка: «всегда переполнено»
    }

    void push(const T& value) {
        (void)value;
        throw std::logic_error("TODO: RingBuffer::push");
    }

    // try_pop объявлен ниже (возвращает std::optional<T>).
    template <class Self = RingBuffer>
    auto try_pop();

    size_type size() const     { return 0; }      // неправильная заглушка
    constexpr size_type capacity() const { return N; }
    bool empty() const         { return true; }   // неправильная заглушка
    bool full()  const         { return true; }   // неправильная заглушка

private:
    T         buf_[N] {};
    size_type head_ = 0;   // индекс следующего на чтение
    size_type tail_ = 0;   // индекс следующего на запись
    size_type size_ = 0;
};

// ---------------------------------------------------------------------
// Вынесенные определения шаблонных методов, возвращающих std::optional.
// Их тоже нужно реализовать.
// ---------------------------------------------------------------------
#include <optional>

template <class K, class V>
template <class Self>
auto LRUCache<K, V>::get(const K& key) {
    (void)key;
    // Неправильная заглушка: всегда «промах».
    return std::optional<V>{std::nullopt};
}

template <class T, std::size_t N>
template <class Self>
auto RingBuffer<T, N>::try_pop() {
    // Неправильная заглушка: всегда «пусто».
    return std::optional<T>{std::nullopt};
}
