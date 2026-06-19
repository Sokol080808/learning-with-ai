#include "task06.hpp"
#include <utility>  // std::move

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
