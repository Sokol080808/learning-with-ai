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
        // Эталонная реализация: разрушаем живые элементы и возвращаем сырьё.
        clear_and_free();
    }

    // Запрещаем копирование/перемещение по умолчанию, пока ты не реализуешь
    // их сам (иначе компилятор сгенерировал бы поверхностные версии,
    // которые двойным free сломали бы ручное управление памятью).
    Vector(const Vector&)            = delete;
    Vector& operator=(const Vector&) = delete;

    void push_back(const T& value) {
        if (size_ == cap_)
            reserve(cap_ == 0 ? 1 : cap_ * 2);
        new (data_ + size_) T(value);   // placement new в сырой ячейке
        ++size_;
    }

    void pop_back() {
        --size_;
        data_[size_].~T();              // явно разрушаем последний элемент
    }

    T& operator[](size_type i)             { return data_[i]; }
    const T& operator[](size_type i) const { return data_[i]; }

    T& at(size_type i) {
        if (i >= size_) throw std::out_of_range("Vector::at");
        return data_[i];
    }
    const T& at(size_type i) const {
        if (i >= size_) throw std::out_of_range("Vector::at");
        return data_[i];
    }

    size_type size() const     { return size_; }
    size_type capacity() const { return cap_; }
    bool empty() const         { return size_ == 0; }

    iterator       begin()        { return data_; }
    iterator       end()          { return data_ + size_; }
    const_iterator begin() const  { return data_; }
    const_iterator end()   const  { return data_ + size_; }

private:
    // Выделяет новый блок ёмкостью new_cap, переносит туда живые элементы,
    // освобождает старый блок. new_cap всегда строго больше size_.
    void reserve(size_type new_cap) {
        T* raw = static_cast<T*>(::operator new(new_cap * sizeof(T)));
        for (size_type i = 0; i < size_; ++i) {
            new (raw + i) T(std::move(data_[i]));  // переносим
            data_[i].~T();                         // разрушаем исходник
        }
        ::operator delete(data_);
        data_ = raw;
        cap_  = new_cap;
    }

    void clear_and_free() {
        for (size_type i = 0; i < size_; ++i)
            data_[i].~T();
        ::operator delete(data_);
        data_ = nullptr;
        size_ = cap_ = 0;
    }

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
        // Эталонная реализация: heap-блок (если есть) выделен через new[],
        // потому освобождаем его симметрично через delete[]. Встроенный
        // буфер разрушится сам как обычное поле объекта.
        delete[] heap_;
    }

    SmallVector(const SmallVector&)            = delete;
    SmallVector& operator=(const SmallVector&) = delete;

    void push_back(const T& value) {
        if (size_ == cap_)
            grow();
        data()[size_] = value;
        ++size_;
    }
    void pop_back() {
        --size_;
        data()[size_] = T{};   // «забываем» хвост (логически снят)
    }

    T& operator[](size_type i)             { return data()[i]; }
    const T& operator[](size_type i) const { return data()[i]; }

    size_type size() const     { return size_; }
    size_type capacity() const { return cap_; }
    bool on_heap() const       { return heap_ != nullptr; }

private:
    // Единая точка истины: где сейчас лежат данные.
    T*       data()       { return heap_ ? heap_ : inline_; }
    const T* data() const { return heap_ ? heap_ : inline_; }

    // Переезжаем в кучу побольше (или растим уже-кучный буфер ×2).
    void grow() {
        size_type new_cap = cap_ == 0 ? 1 : cap_ * 2;
        T* fresh = new T[new_cap];
        T* old   = data();
        for (size_type i = 0; i < size_; ++i)
            fresh[i] = std::move(old[i]);
        delete[] heap_;        // nullptr при первом переезде из inline_ — безопасно
        heap_ = fresh;
        cap_  = new_cap;
    }

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
        node->next = nullptr;
        node->prev = tail_;
        if (tail_) tail_->next = node;
        else       head_ = node;     // список был пуст — узел стал и головой
        tail_ = node;
        ++size_;
    }

    void erase(Node<T>* node) {
        if (node->prev) node->prev->next = node->next;
        else            head_ = node->next;   // node — голова
        if (node->next) node->next->prev = node->prev;
        else            tail_ = node->prev;   // node — хвост
        node->next = node->prev = nullptr;     // гигиена вынутого узла
        --size_;
    }

    size_type size() const { return size_; }
    bool empty() const     { return size_ == 0; }

    T& front() { return head_->value; }
    T& back()  { return tail_->value; }

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

    explicit LRUCache(size_type capacity) : capacity_(capacity) {
        if (capacity == 0)
            throw std::out_of_range("LRUCache: capacity must be > 0");
    }

    void put(const K& key, const V& value) {
        auto it = index_.find(key);
        if (it != index_.end()) {
            // Ключ уже есть: обновляем значение и делаем его самым свежим.
            it->second->second = value;
            order_.splice(order_.begin(), order_, it->second);
            return;
        }
        // Новый ключ — вставляем во front.
        order_.emplace_front(key, value);
        index_[key] = order_.begin();
        // Переполнение — вытесняем самый давно использованный (back).
        if (index_.size() > capacity_) {
            const auto& victim = order_.back();
            index_.erase(victim.first);
            order_.pop_back();
        }
    }

    // Если ключа нет — std::nullopt.
    // (Возвращаем по значению, чтобы не зависеть от <optional> в сигнатуре
    //  заглушки; см. определение get ниже.)
    bool contains(const K& key) const {
        return index_.find(key) != index_.end();
    }

    size_type size() const     { return index_.size(); }
    size_type capacity() const { return capacity_; }

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
        if (full()) return false;
        buf_[tail_] = value;
        tail_ = (tail_ + 1) % N;
        ++size_;
        return true;
    }

    void push(const T& value) {
        if (!try_push(value))
            throw std::out_of_range("RingBuffer::push: buffer is full");
    }

    // try_pop объявлен ниже (возвращает std::optional<T>).
    template <class Self = RingBuffer>
    auto try_pop();

    size_type size() const     { return size_; }
    constexpr size_type capacity() const { return N; }
    bool empty() const         { return size_ == 0; }
    bool full()  const         { return size_ == N; }

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
    auto it = index_.find(key);
    if (it == index_.end())
        return std::optional<V>{std::nullopt};
    // Попадание: делаем ключ самым свежим и возвращаем значение.
    order_.splice(order_.begin(), order_, it->second);
    return std::optional<V>{it->second->second};
}

template <class T, std::size_t N>
template <class Self>
auto RingBuffer<T, N>::try_pop() {
    if (empty())
        return std::optional<T>{std::nullopt};
    std::optional<T> result{buf_[head_]};
    head_ = (head_ + 1) % N;
    --size_;
    return result;
}
