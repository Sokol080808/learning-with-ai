#include "task13.hpp"
// #include <unordered_map>

int binary_search_idx(const std::vector<int>& sorted, int target) {
    // TODO: бинарный поиск, верни индекс или -1
    (void)sorted; (void)target;
    return -1;
}

std::vector<int> insertion_sort(std::vector<int> v) {
    // TODO: отсортируй v по возрастанию своей реализацией, верни v
    return v;
}

std::optional<std::pair<int, int>> two_sum(const std::vector<int>& v, int target) {
    // TODO: O(n) через unordered_map (значение -> индекс)
    (void)v; (void)target;
    return std::nullopt;
}

// ── Задание 7. BST ─────────────────────────────────────────────────────────
// Подсказка по структуре: рекурсия удобно работает с std::unique_ptr<Node>&.
// Можешь завести приватные свободные хелперы в этом .cpp.

void BST::insert(int value) {
    // TODO: спускайся от корня; меньше — налево, больше — направо;
    //       равно — ничего не делать (дубликат). На пустом месте — создать узел,
    //       не забудь увеличить size_.
    (void)value;
}

bool BST::contains(int value) const {
    // TODO: спускайся от корня, сравнивая value с узлами.
    (void)value;
    return false;
}

std::vector<int> BST::inorder() const {
    // TODO: рекурсивный обход лево-корень-право складывает значения по возрастанию.
    return {};
}

std::size_t BST::size() const {
    // TODO
    return 0;
}
