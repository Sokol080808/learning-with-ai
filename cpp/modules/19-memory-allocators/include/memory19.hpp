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
        // TODO: выровнять cursor_ вверх до кратного align, проверить, что
        // выровненный блок + n помещается в [begin_, begin_+capacity_), сдвинуть
        // cursor_ и вернуть начало блока. При нехватке — throw std::bad_alloc{}.
        (void)n; (void)align;
        throw std::logic_error("TODO: BumpAllocator::allocate");
    }

    void reset() {
        // TODO: вернуть cursor_ в begin_.
        throw std::logic_error("TODO: BumpAllocator::reset");
    }

    std::size_t used() const {
        // TODO: вернуть число занятых байт.
        return 99999;  // заведомо неверно
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

    UniquePtr(UniquePtr&& other) noexcept : ptr_(nullptr) {
        // TODO: украсть указатель у other, оставив other пустым.
        // (Заглушка ничего не крадёт — указатель остаётся nullptr.)
        (void)other;
    }

    UniquePtr& operator=(UniquePtr&& other) noexcept {
        // TODO: освободить свой объект, забрать чужой указатель, обнулить other.
        // Не забудь про самоприсваивание. (Заглушка ничего не делает.)
        (void)other;
        return *this;
    }

    ~UniquePtr() {
        // TODO: delete ptr_;  (delete nullptr безопасен)
        // (Заглушка не удаляет объект — деструктор объекта не вызовется.)
    }

    T* get() const noexcept { return ptr_; }

    T* release() noexcept {
        // TODO: вернуть текущий указатель, а внутри обнулиться (владение отдано).
        // (Заглушка возвращает nullptr и владение не отдаёт.)
        return nullptr;
    }

    void reset(T* p = nullptr) noexcept {
        // TODO: удалить старый объект и принять новый указатель p.
        // (Заглушка ничего не меняет.)
        (void)p;
    }

    T& operator*()  const { return *ptr_; }
    T* operator->() const noexcept { return ptr_; }

    explicit operator bool() const noexcept {
        // TODO: true, если владеем объектом.
        return false;  // заведомо неверно
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
        // TODO: создать ControlBlock<T>, записать ptr=p, strong=1, weak=0.
        // При исключении p не должен утечь (для простоты можно: new ControlBlock,
        // затем присвоить поля).
        // (Заглушка не создаёт контрол-блок — указатель p при этом теряется.)
        (void)p;
    }

    SharedPtr(const SharedPtr& other) noexcept : ctrl_(other.ctrl_) {
        // TODO: если ctrl_ != nullptr — увеличить strong.
        // (Заглушка не увеличивает счётчик.)
    }

    SharedPtr(SharedPtr&& other) noexcept : ctrl_(nullptr) {
        // TODO: украсть ctrl_ у other (other становится пустым).
        // (Заглушка ничего не крадёт.)
        (void)other;
    }

    SharedPtr& operator=(const SharedPtr& other) noexcept {
        // TODO: release_() своего, скопировать ctrl_, ++strong. Учти самоприсваивание.
        // (Заглушка ничего не делает.)
        (void)other;
        return *this;
    }

    SharedPtr& operator=(SharedPtr&& other) noexcept {
        // TODO: release_() своего, забрать ctrl_, обнулить other.
        // (Заглушка ничего не делает.)
        (void)other;
        return *this;
    }

    ~SharedPtr() {
        // TODO: release_();
        // (Заглушка не уменьшает счётчики — объект не разрушится.)
    }

    long use_count() const noexcept {
        // TODO: вернуть ctrl_ ? ctrl_->strong : 0.
        return -1;  // заведомо неверно
    }

    T*  get() const noexcept { return ctrl_ ? ctrl_->ptr : nullptr; }
    T&  operator*()  const { return *get(); }
    T*  operator->() const noexcept { return get(); }
    explicit operator bool() const noexcept { return get() != nullptr; }

private:
    // Уменьшить strong; при strong==0 удалить объект; при strong==0 && weak==0
    // удалить контрол-блок. (Вызывается из деструктора и присваиваний.)
    void release_() {
        // TODO: реализовать описанную выше логику. Подсказка: если ctrl_ == nullptr
        // — ничего не делать. После --strong, если strong == 0 — delete ctrl_->ptr
        // (и обнули его); если ещё и weak == 0 — delete ctrl_.
    }

    ControlBlock<T>* ctrl_;

    friend class WeakPtr<T>;
};

template <class T>
class WeakPtr {
public:
    WeakPtr() noexcept : ctrl_(nullptr) {}

    WeakPtr(const SharedPtr<T>& sp) noexcept : ctrl_(sp.ctrl_) {
        // TODO: если ctrl_ != nullptr — увеличить weak.
        // (Заглушка не увеличивает счётчик.)
    }

    WeakPtr(const WeakPtr& other) noexcept : ctrl_(other.ctrl_) {
        // TODO: если ctrl_ != nullptr — увеличить weak.
        // (Заглушка не увеличивает счётчик.)
    }

    WeakPtr(WeakPtr&& other) noexcept : ctrl_(nullptr) {
        // TODO: украсть ctrl_ у other.
        // (Заглушка ничего не крадёт.)
        (void)other;
    }

    WeakPtr& operator=(const WeakPtr& other) noexcept {
        // TODO: release_() своего, скопировать ctrl_, ++weak. Учти самоприсваивание.
        // (Заглушка ничего не делает.)
        (void)other;
        return *this;
    }

    WeakPtr& operator=(WeakPtr&& other) noexcept {
        // TODO: release_() своего, забрать ctrl_, обнулить other.
        // (Заглушка ничего не делает.)
        (void)other;
        return *this;
    }

    ~WeakPtr() {
        // TODO: release_();
        // (Заглушка ничего не делает.)
    }

    bool expired() const noexcept {
        // TODO: true, если объекта уже нет (ctrl_ == nullptr || strong == 0).
        return false;  // заведомо неверно
    }

    // Повыситься до SharedPtr. Если объект жив (strong > 0) — вернуть владеющий
    // SharedPtr (strong увеличится). Иначе — пустой SharedPtr.
    SharedPtr<T> lock() const noexcept {
        // TODO: если !expired() — собрать SharedPtr, разделяющий этот ctrl_,
        // увеличив strong; иначе вернуть пустой.
        // (Заглушка всегда возвращает пустой SharedPtr.)
        return SharedPtr<T>();
    }

private:
    // Уменьшить weak; если strong==0 && weak==0 — удалить контрол-блок.
    void release_() {
        // TODO: если ctrl_ == nullptr — выходим. Иначе --weak; если strong==0 и
        // weak==0 — delete ctrl_.
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
        // TODO: пройтись по block_count блокам в buffer и связать их в free-list:
        // в каждый блок записать указатель на следующий свободный, последний — на
        // nullptr. free_head_ должен указывать на первый блок, free_count_ ==
        // block_count. Адрес i-го блока: (std::byte*)buffer + i*block_size.
        (void)buffer; (void)block_count;
    }

    void* allocate() {
        // TODO: если free_head_ == nullptr — вернуть nullptr. Иначе снять голову
        // списка: запомнить её, сдвинуть free_head_ на следующий блок, уменьшить
        // free_count_, вернуть снятый блок.
        throw std::logic_error("TODO: PoolAllocator::allocate");
    }

    void deallocate(void* p) {
        // TODO: вернуть блок p в голову free-list: записать в первые байты p
        // текущий free_head_, затем free_head_ = p, увеличить free_count_.
        (void)p;
        throw std::logic_error("TODO: PoolAllocator::deallocate");
    }

    std::size_t free_count() const {
        // TODO: вернуть число свободных блоков.
        return 999999;  // заведомо неверно
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
    // TODO: вернуть new (raw) T(std::forward<Args>(args)...);
    (void)raw;
    throw std::logic_error("TODO: construct_at19");
}

template <class T>
void destroy_at19(T* p) {
    // TODO: вызвать деструктор объекта: p->~T();  (память при этом НЕ освобождаем)
    (void)p;
    throw std::logic_error("TODO: destroy_at19");
}

}  // namespace m19
