#include "int_vector.hpp"
#include <stdexcept>  // std::out_of_range
#include <utility>    // std::swap, std::move

// Это эталонная реализация (answer key) — на reference-ветке, учащимся не отдаётся.
// Внимание к памяти: каждому new[] соответствует ровно один delete[].

IntVector::IntVector()
    : data_(nullptr), size_(0), capacity_(0) {
    // пустой вектор — ничего выделять не нужно
}

IntVector::IntVector(std::size_t count, int value)
    : data_(count ? new int[count] : nullptr), size_(count), capacity_(count) {
    for (std::size_t i = 0; i < size_; ++i) data_[i] = value;
}

IntVector::~IntVector() {
    delete[] data_;
}

IntVector::IntVector(const IntVector& other)
    : data_(other.size_ ? new int[other.size_] : nullptr),
      size_(other.size_),
      capacity_(other.size_) {
    for (std::size_t i = 0; i < size_; ++i) data_[i] = other.data_[i];
}

IntVector& IntVector::operator=(const IntVector& other) {
    if (this != &other) {
        // copy-and-swap: сделать глубокую копию, затем обменять внутренности.
        IntVector tmp(other);
        std::swap(data_, tmp.data_);
        std::swap(size_, tmp.size_);
        std::swap(capacity_, tmp.capacity_);
        // старое содержимое *this уезжает в tmp и уничтожается при выходе.
    }
    return *this;
}

IntVector::IntVector(IntVector&& other) noexcept
    : data_(other.data_), size_(other.size_), capacity_(other.capacity_) {
    other.data_ = nullptr;
    other.size_ = 0;
    other.capacity_ = 0;
}

IntVector& IntVector::operator=(IntVector&& other) noexcept {
    if (this != &other) {
        delete[] data_;
        data_ = other.data_;
        size_ = other.size_;
        capacity_ = other.capacity_;
        other.data_ = nullptr;
        other.size_ = 0;
        other.capacity_ = 0;
    }
    return *this;
}

void IntVector::push_back(int value) {
    if (size_ == capacity_) {
        std::size_t new_cap = capacity_ == 0 ? 1 : capacity_ * 2;
        int* new_data = new int[new_cap];
        for (std::size_t i = 0; i < size_; ++i) new_data[i] = data_[i];
        delete[] data_;
        data_ = new_data;
        capacity_ = new_cap;
    }
    data_[size_++] = value;
}

std::size_t IntVector::size() const {
    return size_;
}

std::size_t IntVector::capacity() const {
    return capacity_;
}

bool IntVector::empty() const {
    return size_ == 0;
}

int& IntVector::operator[](std::size_t i) {
    return data_[i];
}

const int& IntVector::operator[](std::size_t i) const {
    return data_[i];
}

// ───────────────────────────────────────────────────────────────────────────
// Задание 2: Buffer — стабы (тесты красные)
// ───────────────────────────────────────────────────────────────────────────

Buffer::Buffer(std::size_t count, double value)
    : data_(count ? new double[count] : nullptr), size_(count) {
    for (std::size_t i = 0; i < size_; ++i) data_[i] = value;
}

Buffer::~Buffer() {
    delete[] data_;
}

Buffer::Buffer(const Buffer& other)
    : data_(other.size_ ? new double[other.size_] : nullptr),
      size_(other.size_) {
    for (std::size_t i = 0; i < size_; ++i) data_[i] = other.data_[i];
}

Buffer& Buffer::operator=(const Buffer& other) {
    // copy-and-swap: сделать глубокую копию, затем обменять с ней внутренности.
    // Безопасно при самоприсваивании и при исключении в копировании.
    Buffer tmp(other);
    swap(tmp);
    return *this;
}

Buffer::Buffer(Buffer&& other) noexcept
    : data_(other.data_), size_(other.size_) {
    other.data_ = nullptr;
    other.size_ = 0;
}

Buffer& Buffer::operator=(Buffer&& other) noexcept {
    if (this != &other) {
        delete[] data_;
        data_ = other.data_;
        size_ = other.size_;
        other.data_ = nullptr;
        other.size_ = 0;
    }
    return *this;
}

std::size_t Buffer::size() const {
    return size_;
}

double* Buffer::data() {
    return data_;
}

const double* Buffer::data() const {
    return data_;
}

double& Buffer::operator[](std::size_t i) {
    return data_[i];
}

const double& Buffer::operator[](std::size_t i) const {
    return data_[i];
}

double& Buffer::at(std::size_t i) {
    if (i >= size_) throw std::out_of_range("Buffer::at: index out of range");
    return data_[i];
}

const double& Buffer::at(std::size_t i) const {
    if (i >= size_) throw std::out_of_range("Buffer::at: index out of range");
    return data_[i];
}

void Buffer::fill(double value) {
    for (std::size_t i = 0; i < size_; ++i) data_[i] = value;
}

void Buffer::swap(Buffer& other) noexcept {
    std::swap(data_, other.data_);
    std::swap(size_, other.size_);
}

// ───────────────────────────────────────────────────────────────────────────
// Задание 3: UniqueHandle + учебный реестр ресурсов — стабы
// ───────────────────────────────────────────────────────────────────────────
//
// Реестр УЖЕ реализован — его трогать не нужно, он нужен тестам для проверки
// «нет утечек / нет двойного release». Реализуй только UniqueHandle и make_handle.

namespace {
int g_next_id = 1;       // следующий выдаваемый id
int g_live_count = 0;    // сколько id сейчас живо
}

int resource_acquire() {
    ++g_live_count;
    return g_next_id++;
}

void resource_release(int id) {
    if (id == kInvalidHandle) return;          // пустой хэндл — нечего освобождать
    if (g_live_count <= 0) {
        // двойное освобождение/освобождение лишнего — грубая ошибка
        throw std::logic_error("resource_release: double free / unbalanced release");
    }
    --g_live_count;
}

int resource_live_count() {
    return g_live_count;
}

void resource_reset_registry() {
    g_next_id = 1;
    g_live_count = 0;
}

UniqueHandle::UniqueHandle() noexcept
    : id_(kInvalidHandle) {
    // пустой хэндл
}

UniqueHandle::UniqueHandle(int id) noexcept
    : id_(id) {
    // берём готовый id во владение
}

UniqueHandle::~UniqueHandle() {
    if (id_ != kInvalidHandle) resource_release(id_);
}

UniqueHandle::UniqueHandle(UniqueHandle&& other) noexcept
    : id_(other.id_) {
    other.id_ = kInvalidHandle;   // источник больше ничем не владеет
}

UniqueHandle& UniqueHandle::operator=(UniqueHandle&& other) noexcept {
    // reset(other.release()): забрать чужой id (обнулив other), заменить им свой.
    // Естественно переживает self-move: release() вынет id, reset вернёт его назад.
    reset(other.release());
    return *this;
}

int UniqueHandle::get() const noexcept {
    return id_;
}

bool UniqueHandle::valid() const noexcept {
    return id_ != kInvalidHandle;
}

int UniqueHandle::release() noexcept {
    int id = id_;
    id_ = kInvalidHandle;   // отдаём наружу, ресурс НЕ освобождаем
    return id;
}

void UniqueHandle::reset(int id) noexcept {
    if (id_ != kInvalidHandle) resource_release(id_);
    id_ = id;
}

void UniqueHandle::swap(UniqueHandle& other) noexcept {
    std::swap(id_, other.id_);
}

UniqueHandle make_handle() {
    return UniqueHandle{resource_acquire()};
}
