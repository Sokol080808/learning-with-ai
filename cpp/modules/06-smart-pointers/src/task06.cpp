#include "task06.hpp"
#include <optional>  // std::optional, std::nullopt
#include <utility>   // std::move
#include <stdexcept> // std::logic_error

std::unique_ptr<int> make_unique_int(int v) {
    // TODO: используй std::make_unique
    (void)v;
    return nullptr;
}

void IntList::push_front(int value) {
    // TODO: создать узел, перецепить head_
    (void)value;
}

std::size_t IntList::size() const {
    // TODO: пройти по списку и посчитать узлы
    return 0;
}

bool IntList::contains(int value) const {
    // TODO
    (void)value;
    return false;
}

std::vector<int> IntList::to_vector() const {
    // TODO: собрать значения от головы к хвосту
    return {};
}

// ---------------------------------------------------------------------------
// Задание 3. Дерево (shared_ptr на детей, weak_ptr на родителя).

std::shared_ptr<TreeNode> TreeNode::add_child(int child_value) {
    // TODO: создать make_shared<TreeNode>(child_value),
    //       выставить ему parent_ = weak_ptr на this (через shared_from_this()),
    //       положить в children_ и вернуть нового ребёнка.
    (void)child_value;
    throw std::logic_error("TODO: TreeNode::add_child");
}

std::size_t TreeNode::child_count() const {
    // TODO: вернуть размер children_
    return 0;
}

std::shared_ptr<TreeNode> TreeNode::child(std::size_t i) const {
    // TODO: вернуть children_[i]
    (void)i;
    return nullptr;
}

std::shared_ptr<TreeNode> TreeNode::parent() const {
    // TODO: повысить weak_ptr через lock() (для корня вернётся nullptr)
    return nullptr;
}

int TreeNode::subtree_sum() const {
    // TODO: value_ + сумма subtree_sum() по всем детям (рекурсивно)
    return 0;
}

std::shared_ptr<TreeNode> make_tree_root(int value) {
    // TODO: вернуть make_shared<TreeNode>(value)
    (void)value;
    throw std::logic_error("TODO: make_tree_root");
}

// ---------------------------------------------------------------------------
// Задание 4. Pimpl на unique_ptr.
//
// Полное определение Impl живёт здесь, в .cpp — заголовок его не видит.
struct Counter::Impl {
    int value = 0;
};

Counter::Counter(int start) : impl_(std::make_unique<Impl>()) {
    // TODO: записать start в impl_->value
    (void)start;
}

// Деструктор и операции перемещения определяются ЗДЕСЬ, где Impl — полный тип.
// Тело может быть дефолтным: компилятор сгенерирует корректное удаление unique_ptr<Impl>.
Counter::~Counter() = default;
Counter::Counter(Counter&&) noexcept = default;
Counter& Counter::operator=(Counter&&) noexcept = default;

void Counter::increment() {
    // TODO: impl_->value += 1
}

void Counter::add(int delta) {
    // TODO: impl_->value += delta
    (void)delta;
}

int Counter::value() const {
    // TODO: вернуть impl_->value
    return -1;
}

// ---------------------------------------------------------------------------
// Задание 5. Общий ресурс и use_count.

SharedResource::SharedResource(std::string data) {
    // TODO: data_ = std::make_shared<std::string>(std::move(data));
    (void)data;
}

long SharedResource::use_count() const {
    // TODO: вернуть data_.use_count()
    return 0;
}

std::string SharedResource::read() const {
    // TODO: вернуть *data_
    return {};
}

SharedResource SharedResource::share() const {
    // TODO: вернуть копию *this так, чтобы оба объекта владели одним shared-блоком.
    throw std::logic_error("TODO: SharedResource::share");
}

// ---------------------------------------------------------------------------
// Задание 6. maybe_at: безопасный доступ к элементу вектора через optional.

std::optional<int> maybe_at(const std::vector<int>& v, std::size_t i) {
    // TODO: если i < v.size() — вернуть std::optional{v[i]}, иначе std::nullopt.
    (void)v;
    (void)i;
    return std::nullopt;  // заглушка: всегда пусто — тесты с валидными индексами упадут
}
