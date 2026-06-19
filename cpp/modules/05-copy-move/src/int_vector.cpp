#include "int_vector.hpp"
#include <stdexcept>  // std::logic_error, std::out_of_range
// #include <algorithm>  // может пригодиться std::copy / std::swap

// Реализуй методы IntVector. Сейчас всё — стабы (тесты красные).
// Внимание к памяти: каждому new[] должен соответствовать ровно один delete[].

IntVector::IntVector()
    : data_(nullptr), size_(0), capacity_(0) {
    // пустой вектор — ничего выделять не нужно
}

IntVector::IntVector(std::size_t count, int value)
    : data_(nullptr), size_(0), capacity_(0) {
    // TODO: выдели массив на count элементов и заполни значением value
    (void)count; (void)value;
}

IntVector::~IntVector() {
    // TODO: освободи память
}

IntVector::IntVector(const IntVector& other)
    : data_(nullptr), size_(0), capacity_(0) {
    // TODO: глубокое копирование из other
    (void)other;
}

IntVector& IntVector::operator=(const IntVector& other) {
    // TODO: безопасно (учти самоприсваивание; освободи старое)
    (void)other;
    return *this;
}

IntVector::IntVector(IntVector&& other) noexcept
    : data_(nullptr), size_(0), capacity_(0) {
    // TODO: забери ресурсы у other, оставь other пустым
    (void)other;
}

IntVector& IntVector::operator=(IntVector&& other) noexcept {
    // TODO
    (void)other;
    return *this;
}

void IntVector::push_back(int value) {
    // TODO: при необходимости вырасти, затем добавить элемент
    (void)value;
}

std::size_t IntVector::size() const {
    return size_;
}

std::size_t IntVector::capacity() const {
    return capacity_;
}

bool IntVector::empty() const {
    // TODO
    return true;
}

int& IntVector::operator[](std::size_t i) {
    // TODO
    return data_[i];
}

const int& IntVector::operator[](std::size_t i) const {
    // TODO
    return data_[i];
}

// ───────────────────────────────────────────────────────────────────────────
// Задание 2: Buffer — стабы (тесты красные)
// ───────────────────────────────────────────────────────────────────────────

Buffer::Buffer(std::size_t count, double value)
    : data_(nullptr), size_(0) {
    // TODO: выдели count элементов через new[] и заполни значением value.
    //       Помни: count == 0 — корректное состояние (data_ == nullptr).
    (void)count; (void)value;
}

Buffer::~Buffer() {
    // TODO: освободи буфер (delete[]). delete[] nullptr — безопасен.
}

Buffer::Buffer(const Buffer& other)
    : data_(nullptr), size_(0) {
    // TODO: ГЛУБОКАЯ копия — свой буфер, поэлементное копирование.
    (void)other;
}

Buffer& Buffer::operator=(const Buffer& other) {
    // TODO: безопасное присваивание (самоприсваивание! освобождение старого).
    //       Подумай про copy-and-swap.
    (void)other;
    return *this;
}

Buffer::Buffer(Buffer&& other) noexcept
    : data_(nullptr), size_(0) {
    // TODO: забери буфер у other, оставь other в (nullptr, 0).
    (void)other;
}

Buffer& Buffer::operator=(Buffer&& other) noexcept {
    // TODO
    (void)other;
    return *this;
}

std::size_t Buffer::size() const {
    // TODO
    return 0;
}

double* Buffer::data() {
    // TODO
    return nullptr;
}

const double* Buffer::data() const {
    // TODO
    return nullptr;
}

double& Buffer::operator[](std::size_t i) {
    // TODO: без проверки границ
    (void)i;
    throw std::logic_error("TODO: Buffer::operator[]");
}

const double& Buffer::operator[](std::size_t i) const {
    // TODO
    (void)i;
    throw std::logic_error("TODO: Buffer::operator[] const");
}

double& Buffer::at(std::size_t i) {
    // TODO: при i >= size() бросить std::out_of_range; иначе вернуть элемент.
    (void)i;
    throw std::logic_error("TODO: Buffer::at");
}

const double& Buffer::at(std::size_t i) const {
    // TODO
    (void)i;
    throw std::logic_error("TODO: Buffer::at const");
}

void Buffer::fill(double value) {
    // TODO: записать value во все элементы.
    (void)value;
}

void Buffer::swap(Buffer& other) noexcept {
    // TODO: обменять data_ и size_ местами (std::swap пригодится).
    (void)other;
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
    : id_(kInvalidHandle) {
    // TODO: взять id во владение.
    (void)id;
}

UniqueHandle::~UniqueHandle() {
    // TODO: если владеем ресурсом — освободить его (resource_release).
}

UniqueHandle::UniqueHandle(UniqueHandle&& other) noexcept
    : id_(kInvalidHandle) {
    // TODO: украсть владение у other, обнулить other.
    (void)other;
}

UniqueHandle& UniqueHandle::operator=(UniqueHandle&& other) noexcept {
    // TODO: освободить своё, забрать чужое, обнулить other (учти self-move).
    (void)other;
    return *this;
}

int UniqueHandle::get() const noexcept {
    // TODO
    return id_;
}

bool UniqueHandle::valid() const noexcept {
    // TODO: владеет ли реальным id?
    return false;
}

int UniqueHandle::release() noexcept {
    // TODO: вернуть текущий id, стать пустым (НЕ освобождать ресурс — отдаём наружу).
    return kInvalidHandle;
}

void UniqueHandle::reset(int id) noexcept {
    // TODO: освободить текущий ресурс (если есть), взять id.
    (void)id;
}

void UniqueHandle::swap(UniqueHandle& other) noexcept {
    // TODO: обменять id_.
    (void)other;
}

UniqueHandle make_handle() {
    // TODO: захватить ресурс из реестра и обернуть его в UniqueHandle.
    return UniqueHandle{};
}
