#pragma once
// Модуль 19 — Память, выравнивание, аллокаторы и устройство умных указателей.
//
// Header-only: весь код шаблонов и инлайн-классов живёт здесь. Реализуй тела
// методов прямо в этом файле (заменяя `throw std::logic_error("TODO: ...")` и
// заведомо неверные заглушки на правильную логику).
//
// Разрешён только C++20.

#include <cstddef>      // std::size_t, std::byte, std::max_align_t
#include <cstdint>      // std::uintptr_t
#include <cstring>      // std::memcpy
#include <new>          // placement new, ::operator new/delete
#include <utility>      // std::move, std::exchange, std::swap, std::forward
#include <stdexcept>    // std::logic_error, std::bad_alloc
#include <type_traits>  // std::is_array и т.п.

namespace m19 {

// ─────────────────────────────────────────────────────────────────────────────
// Задание 1. BumpAllocator («указатель-бугор», bump/arena).
//
// Линейный аллокатор над ЧУЖИМ сырым буфером [buffer, buffer + capacity).
// allocate(n, align) сдвигает внутренний курсор вперёд, выравнивая его, и
// возвращает указатель на n байт. Освобождения по одному нет — только reset(),
// который «отматывает» курсор в начало и делает весь буфер свободным снова.
//
// Контракт:
//   * Конструктор принимает (void* buffer, std::size_t capacity).
//   * allocate(n, align): align — степень двойки (1,2,4,...). Курсор сначала
//     выравнивается ВВЕРХ до кратного align, затем сдвигается на n. Если после
//     выравнивания n байт не помещается в буфер — бросить std::bad_alloc.
//     Возвращает указатель на выровненный блок.
//   * used()  — сколько байт занято (от начала буфера до курсора).
//   * reset() — освободить всё: used() снова 0.
//   * capacity() — ёмкость буфера.
// ─────────────────────────────────────────────────────────────────────────────
class BumpAllocator {
public:
    BumpAllocator(void* buffer, std::size_t capacity)
        : begin_(static_cast<std::byte*>(buffer)),
          cursor_(static_cast<std::byte*>(buffer)),
          capacity_(capacity) {}

    // align гарантированно степень двойки. Бросает std::bad_alloc при нехватке.
    void* allocate(std::size_t n, std::size_t align = alignof(std::max_align_t)) {
        // Эталонный ответ (answer key).
        // Считаем адреса в целых, чтобы не делать арифметику над void*.
        const std::uintptr_t cur     = reinterpret_cast<std::uintptr_t>(cursor_);
        const std::uintptr_t aligned = (cur + (align - 1)) & ~(static_cast<std::uintptr_t>(align) - 1);
        const std::uintptr_t end     = reinterpret_cast<std::uintptr_t>(begin_) + capacity_;
        // Сравниваем остаток с n, а не aligned + n с end — иначе на больших n
        // сложение указателей переполнится.
        if (aligned > end || (end - aligned) < n) {
            throw std::bad_alloc{};
        }
        std::byte* result = reinterpret_cast<std::byte*>(aligned);
        cursor_ = result + n;
        return result;
    }

    void reset() {
        // Эталонный ответ (answer key).
        cursor_ = begin_;
    }

    std::size_t used() const {
        // Эталонный ответ (answer key). Занятое = курсор минус начало буфера.
        return static_cast<std::size_t>(cursor_ - begin_);
    }

    std::size_t capacity() const { return capacity_; }

private:
    std::byte*  begin_;
    std::byte*  cursor_;
    std::size_t capacity_;
};

// ─────────────────────────────────────────────────────────────────────────────
// Задание 2. UniquePtr<T> — единоличное владение, move-only.
//
// Упрощённый аналог std::unique_ptr (без удалителей и без T[]). Владеет сырым
// указателем, в деструкторе удаляет объект. Копирование запрещено, перемещение
// передаёт владение.
//
// Контракт:
//   * UniquePtr() и UniquePtr(nullptr) — пустой.
//   * explicit UniquePtr(T*) — берёт владение.
//   * Деструктор: delete ptr_ (на nullptr безопасно).
//   * Конструктор/присваивание перемещением; копирование удалено.
//   * get() — сырой указатель; release() — отдать владение (вернуть указатель,
//     внутри обнулиться); reset(p=nullptr) — удалить старый, взять новый.
//   * operator*(), operator->(), explicit operator bool().
// ─────────────────────────────────────────────────────────────────────────────
template <class T>
class UniquePtr {
public:
    UniquePtr() noexcept : ptr_(nullptr) {}
    UniquePtr(std::nullptr_t) noexcept : ptr_(nullptr) {}
    explicit UniquePtr(T* p) noexcept : ptr_(p) {}

    UniquePtr(const UniquePtr&)            = delete;
    UniquePtr& operator=(const UniquePtr&) = delete;

    UniquePtr(UniquePtr&& other) noexcept : ptr_(std::exchange(other.ptr_, nullptr)) {
        // Эталонный ответ (answer key): украсть указатель, оставив other пустым.
    }

    UniquePtr& operator=(UniquePtr&& other) noexcept {
        // Эталонный ответ (answer key).
        if (this != &other) {
            delete ptr_;                              // убрать свой старый объект
            ptr_ = std::exchange(other.ptr_, nullptr); // забрать чужой, обнулив other
        }
        return *this;
    }

    ~UniquePtr() {
        // Эталонный ответ (answer key): delete nullptr безопасен.
        delete ptr_;
    }

    T* get() const noexcept { return ptr_; }

    T* release() noexcept {
        // Эталонный ответ (answer key): отдать указатель наружу, внутри обнулиться.
        return std::exchange(ptr_, nullptr);
    }

    void reset(T* p = nullptr) noexcept {
        // Эталонный ответ (answer key): удалить старый объект, принять новый.
        delete ptr_;
        ptr_ = p;
    }

    T& operator*()  const { return *ptr_; }
    T* operator->() const noexcept { return ptr_; }

    explicit operator bool() const noexcept {
        // Эталонный ответ (answer key).
        return ptr_ != nullptr;
    }

private:
    T* ptr_;
};

template <class T, class... Args>
UniquePtr<T> make_unique19(Args&&... args) {
    return UniquePtr<T>(new T(std::forward<Args>(args)...));
}

// ─────────────────────────────────────────────────────────────────────────────
// Задание 3. SharedPtr<T> + WeakPtr<T> с общим контрол-блоком.
//
// SharedPtr владеет объектом и счётчиком сильных ссылок (strong). Объект
// удаляется, когда strong падает до 0. WeakPtr — наблюдатель: он держит
// слабую ссылку (weak), не продлевает жизнь объекта, но позволяет узнать, жив
// ли он, и при необходимости «повыситься» до SharedPtr через lock().
//
// Контрол-блок удаляется только когда И strong, И weak станут 0.
//
// Контракт SharedPtr:
//   * SharedPtr() / SharedPtr(nullptr) — пустой, use_count() == 0.
//   * explicit SharedPtr(T*) — создаёт контрол-блок, strong = 1.
//   * Копирование: ++strong; присваивание корректно. Перемещение: крадёт.
//   * use_count() — текущее strong; get(), operator*, operator->, operator bool.
//
// Контракт WeakPtr:
//   * WeakPtr() — пустой; WeakPtr(const SharedPtr&) — ++weak.
//   * Копирование/перемещение/присваивание корректны.
//   * expired() — true, если объект уже мёртв (strong == 0).
//   * lock() — вернуть SharedPtr: если strong > 0, ++strong и владеем объектом;
//     иначе пустой SharedPtr.
// ─────────────────────────────────────────────────────────────────────────────

// Общий контрол-блок. Эту структуру реализовывать не нужно — она задана целиком.
template <class T>
struct ControlBlock {
    T*         ptr    = nullptr;  // управляемый объект
    long       strong = 0;        // число SharedPtr
    long       weak   = 0;        // число WeakPtr
};

template <class T>
class WeakPtr;  // forward

template <class T>
class SharedPtr {
public:
    SharedPtr() noexcept : ctrl_(nullptr) {}
    SharedPtr(std::nullptr_t) noexcept : ctrl_(nullptr) {}

    explicit SharedPtr(T* p) {
        // Эталонный ответ (answer key): новый контрол-блок, strong=1, weak=0.
        ctrl_         = new ControlBlock<T>;
        ctrl_->ptr    = p;
        ctrl_->strong = 1;
        ctrl_->weak   = 0;
    }

    SharedPtr(const SharedPtr& other) noexcept : ctrl_(other.ctrl_) {
        // Эталонный ответ (answer key): ещё один владелец.
        if (ctrl_) ++ctrl_->strong;
    }

    SharedPtr(SharedPtr&& other) noexcept : ctrl_(std::exchange(other.ctrl_, nullptr)) {
        // Эталонный ответ (answer key): перемещение не трогает счётчик.
    }

    SharedPtr& operator=(const SharedPtr& other) noexcept {
        // Эталонный ответ (answer key).
        if (this != &other) {
            release_();
            ctrl_ = other.ctrl_;
            if (ctrl_) ++ctrl_->strong;
        }
        return *this;
    }

    SharedPtr& operator=(SharedPtr&& other) noexcept {
        // Эталонный ответ (answer key).
        if (this != &other) {
            release_();
            ctrl_ = std::exchange(other.ctrl_, nullptr);
        }
        return *this;
    }

    ~SharedPtr() {
        // Эталонный ответ (answer key).
        release_();
    }

    long use_count() const noexcept {
        // Эталонный ответ (answer key).
        return ctrl_ ? ctrl_->strong : 0;
    }

    T*  get() const noexcept { return ctrl_ ? ctrl_->ptr : nullptr; }
    T&  operator*()  const { return *get(); }
    T*  operator->() const noexcept { return get(); }
    explicit operator bool() const noexcept { return get() != nullptr; }

private:
    // Уменьшить strong; при strong==0 удалить объект; при strong==0 && weak==0
    // удалить контрол-блок. (Вызывается из деструктора и присваиваний.)
    void release_() {
        // Эталонный ответ (answer key): логика двух счётчиков.
        if (!ctrl_) return;
        if (--ctrl_->strong == 0) {
            delete ctrl_->ptr;          // объект больше не нужен
            ctrl_->ptr = nullptr;
            if (ctrl_->weak == 0) {
                delete ctrl_;           // и контрол-блок никому не нужен
            }
        }
        ctrl_ = nullptr;
    }

    ControlBlock<T>* ctrl_;

    friend class WeakPtr<T>;
};

template <class T>
class WeakPtr {
public:
    WeakPtr() noexcept : ctrl_(nullptr) {}

    WeakPtr(const SharedPtr<T>& sp) noexcept : ctrl_(sp.ctrl_) {
        // Эталонный ответ (answer key): ещё один наблюдатель.
        if (ctrl_) ++ctrl_->weak;
    }

    WeakPtr(const WeakPtr& other) noexcept : ctrl_(other.ctrl_) {
        // Эталонный ответ (answer key).
        if (ctrl_) ++ctrl_->weak;
    }

    WeakPtr(WeakPtr&& other) noexcept : ctrl_(std::exchange(other.ctrl_, nullptr)) {
        // Эталонный ответ (answer key): перемещение не трогает счётчик.
    }

    WeakPtr& operator=(const WeakPtr& other) noexcept {
        // Эталонный ответ (answer key).
        if (this != &other) {
            release_();
            ctrl_ = other.ctrl_;
            if (ctrl_) ++ctrl_->weak;
        }
        return *this;
    }

    WeakPtr& operator=(WeakPtr&& other) noexcept {
        // Эталонный ответ (answer key).
        if (this != &other) {
            release_();
            ctrl_ = std::exchange(other.ctrl_, nullptr);
        }
        return *this;
    }

    ~WeakPtr() {
        // Эталонный ответ (answer key).
        release_();
    }

    bool expired() const noexcept {
        // Эталонный ответ (answer key).
        return ctrl_ == nullptr || ctrl_->strong == 0;
    }

    // Повыситься до SharedPtr. Если объект жив (strong > 0) — вернуть владеющий
    // SharedPtr (strong увеличится). Иначе — пустой SharedPtr.
    SharedPtr<T> lock() const noexcept {
        // Эталонный ответ (answer key): разделить тот же ctrl_, подняв strong.
        SharedPtr<T> sp;  // пустой
        if (!expired()) {
            sp.ctrl_ = ctrl_;
            ++ctrl_->strong;
        }
        return sp;
    }

private:
    // Уменьшить weak; если strong==0 && weak==0 — удалить контрол-блок.
    void release_() {
        // Эталонный ответ (answer key).
        if (!ctrl_) return;
        --ctrl_->weak;
        if (ctrl_->strong == 0 && ctrl_->weak == 0) {
            delete ctrl_;
        }
        ctrl_ = nullptr;
    }

    ControlBlock<T>* ctrl_;
};

// ─────────────────────────────────────────────────────────────────────────────
// Задание 4. PoolAllocator — пул блоков фиксированного размера на free-list.
//
// Над ЧУЖИМ сырым буфером нарезаем block_count блоков по block_size байт.
// Свободные блоки связаны в односвязный список (free-list): первые
// sizeof(void*) байт свободного блока хранят указатель на следующий свободный
// блок. allocate() снимает голову списка, deallocate() возвращает блок в голову.
//
// Контракт:
//   * PoolAllocator(void* buffer, std::size_t block_size, std::size_t block_count).
//     Гарантируется block_size >= sizeof(void*), и буфер достаточно велик и
//     выровнен для хранения указателей.
//   * allocate(): вернуть указатель на свободный блок или nullptr, если их нет.
//   * deallocate(void* p): вернуть блок p в free-list (p — ранее выданный блок).
//   * free_count(): сколько блоков сейчас свободно.
//   * block_size(): размер блока.
// ─────────────────────────────────────────────────────────────────────────────
class PoolAllocator {
public:
    PoolAllocator(void* buffer, std::size_t block_size, std::size_t block_count)
        : block_size_(block_size), free_head_(nullptr), free_count_(0) {
        // Эталонный ответ (answer key): связать блоки в free-list.
        std::byte* base = static_cast<std::byte*>(buffer);
        // Связываем с конца, чтобы free_head_ указывал на самый первый блок и
        // блоки выдавались в порядке возрастания адресов.
        for (std::size_t i = block_count; i-- > 0; ) {
            void* block = base + i * block_size_;
            std::memcpy(block, &free_head_, sizeof(void*));  // next := текущая голова
            free_head_ = block;
        }
        free_count_ = block_count;
    }

    void* allocate() {
        // Эталонный ответ (answer key): снять голову free-list.
        if (free_head_ == nullptr) return nullptr;
        void* block = free_head_;
        void* next;
        std::memcpy(&next, block, sizeof(void*));  // прочитать «следующего»
        free_head_ = next;
        --free_count_;
        return block;
    }

    void deallocate(void* p) {
        // Эталонный ответ (answer key): вставить блок в голову free-list.
        std::memcpy(p, &free_head_, sizeof(void*));  // p->next := текущая голова
        free_head_ = p;
        ++free_count_;
    }

    std::size_t free_count() const {
        // Эталонный ответ (answer key).
        return free_count_;
    }

    std::size_t block_size() const { return block_size_; }

private:
    std::size_t block_size_;
    void*       free_head_;   // голова списка свободных блоков
    std::size_t free_count_;
};

// ─────────────────────────────────────────────────────────────────────────────
// Задание 5. Размещение объекта в сыром выровненном буфере (placement new).
//
// Дан тип T и СЫРОЙ буфер достаточного размера и выравнивания. Нужно построить в
// нём объект T(args...) через placement new и вернуть указатель. Парная функция
// корректно разрушает объект (явный вызов деструктора), НЕ освобождая память —
// памятью владеет вызывающая сторона (буфер).
//
//   construct_at19<T>(raw, args...) — построить T в raw, вернуть T*.
//   destroy_at19<T>(p)              — вызвать p->~T() (память не трогать).
// ─────────────────────────────────────────────────────────────────────────────
template <class T, class... Args>
T* construct_at19(void* raw, Args&&... args) {
    // Эталонный ответ (answer key): placement new — память не выделяется.
    return new (raw) T(std::forward<Args>(args)...);
}

template <class T>
void destroy_at19(T* p) {
    // Эталонный ответ (answer key): явный деструктор, память не освобождаем.
    p->~T();
}

}  // namespace m19
