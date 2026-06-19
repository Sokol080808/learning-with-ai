#include "task06.hpp"
#include <utility>   // std::move
#include <stdexcept> // std::logic_error

// Это эталонный ключ ответов (reference answer key) — не отправляется учащимся.

std::unique_ptr<int> make_unique_int(int v) {
    return std::make_unique<int>(v);
}

void IntList::push_front(int value) {
    auto node = std::make_unique<Node>();
    node->value = value;
    node->next = std::move(head_);
    head_ = std::move(node);
}

std::size_t IntList::size() const {
    std::size_t count = 0;
    for (const Node* p = head_.get(); p != nullptr; p = p->next.get()) {
        ++count;
    }
    return count;
}

bool IntList::contains(int value) const {
    for (const Node* p = head_.get(); p != nullptr; p = p->next.get()) {
        if (p->value == value) {
            return true;
        }
    }
    return false;
}

std::vector<int> IntList::to_vector() const {
    std::vector<int> result;
    for (const Node* p = head_.get(); p != nullptr; p = p->next.get()) {
        result.push_back(p->value);
    }
    return result;
}

// ---------------------------------------------------------------------------
// Задание 3. Дерево (shared_ptr на детей, weak_ptr на родителя).

std::shared_ptr<TreeNode> TreeNode::add_child(int child_value) {
    auto child = std::make_shared<TreeNode>(child_value);
    child->parent_ = shared_from_this();
    children_.push_back(child);
    return child;
}

std::size_t TreeNode::child_count() const {
    return children_.size();
}

std::shared_ptr<TreeNode> TreeNode::child(std::size_t i) const {
    return children_[i];
}

std::shared_ptr<TreeNode> TreeNode::parent() const {
    return parent_.lock();
}

int TreeNode::subtree_sum() const {
    int sum = value_;
    for (const auto& child : children_) {
        sum += child->subtree_sum();
    }
    return sum;
}

std::shared_ptr<TreeNode> make_tree_root(int value) {
    return std::make_shared<TreeNode>(value);
}

// ---------------------------------------------------------------------------
// Задание 4. Pimpl на unique_ptr.
//
// Полное определение Impl живёт здесь, в .cpp — заголовок его не видит.
struct Counter::Impl {
    int value = 0;
};

Counter::Counter(int start) : impl_(std::make_unique<Impl>()) {
    impl_->value = start;
}

// Деструктор и операции перемещения определяются ЗДЕСЬ, где Impl — полный тип.
// Тело может быть дефолтным: компилятор сгенерирует корректное удаление unique_ptr<Impl>.
Counter::~Counter() = default;
Counter::Counter(Counter&&) noexcept = default;
Counter& Counter::operator=(Counter&&) noexcept = default;

void Counter::increment() {
    impl_->value += 1;
}

void Counter::add(int delta) {
    impl_->value += delta;
}

int Counter::value() const {
    return impl_->value;
}

// ---------------------------------------------------------------------------
// Задание 5. Общий ресурс и use_count.

SharedResource::SharedResource(std::string data)
    : data_(std::make_shared<std::string>(std::move(data))) {}

long SharedResource::use_count() const {
    return data_.use_count();
}

std::string SharedResource::read() const {
    return *data_;
}

SharedResource SharedResource::share() const {
    return *this;
}
