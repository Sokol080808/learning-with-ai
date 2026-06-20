#include "task13.hpp"
#include <unordered_map>

int binary_search_idx(const std::vector<int>& sorted, int target) {
    int lo = 0;
    int hi = static_cast<int>(sorted.size()) - 1;
    while (lo <= hi) {
        int mid = lo + (hi - lo) / 2;  // без переполнения, в отличие от (lo+hi)/2
        if (sorted[mid] == target) {
            return mid;
        } else if (sorted[mid] < target) {
            lo = mid + 1;
        } else {
            hi = mid - 1;
        }
    }
    return -1;
}

std::vector<int> insertion_sort(std::vector<int> v) {
    // Сортировка вставками: для каждого элемента сдвигаем больших соседей вправо
    // и вставляем элемент на его место в уже отсортированном префиксе.
    for (std::size_t i = 1; i < v.size(); ++i) {
        int key = v[i];
        std::size_t j = i;
        while (j > 0 && v[j - 1] > key) {
            v[j] = v[j - 1];
            --j;
        }
        v[j] = key;
    }
    return v;
}

std::optional<std::pair<int, int>> two_sum(const std::vector<int>& v, int target) {
    // Один проход: для v[i] ищем дополнение (target - v[i]) среди ранее виденных.
    std::unordered_map<int, int> seen;  // значение -> индекс
    for (int i = 0; i < static_cast<int>(v.size()); ++i) {
        int complement = target - v[i];
        auto it = seen.find(complement);
        if (it != seen.end()) {
            return std::make_pair(it->second, i);  // i < текущего по построению
        }
        seen.emplace(v[i], i);
    }
    return std::nullopt;
}

// ── Задание 9. Merge sort — divide-and-conquer, O(n log n) ────────────────
// Рекуррентность: T(n) = 2T(n/2) + O(n) → O(n log n) по теореме Мастера.
// Пространство: O(n) — буфер для слияния (две половины копируются в tmp).

static void merge(std::vector<int>& v, std::size_t lo, std::size_t mid, std::size_t hi) {
    // Копируем обе половины во временный буфер
    std::vector<int> left(v.begin() + static_cast<std::ptrdiff_t>(lo),
                          v.begin() + static_cast<std::ptrdiff_t>(mid));
    std::vector<int> right(v.begin() + static_cast<std::ptrdiff_t>(mid),
                           v.begin() + static_cast<std::ptrdiff_t>(hi));
    std::size_t i = 0, j = 0, k = lo;
    // Слияние двух отсортированных половин
    while (i < left.size() && j < right.size()) {
        if (left[i] <= right[j]) {
            v[k++] = left[i++];
        } else {
            v[k++] = right[j++];
        }
    }
    while (i < left.size())  v[k++] = left[i++];
    while (j < right.size()) v[k++] = right[j++];
}

static void merge_sort_impl(std::vector<int>& v, std::size_t lo, std::size_t hi) {
    if (hi - lo <= 1) return;  // базовый случай: 0 или 1 элемент
    std::size_t mid = lo + (hi - lo) / 2;
    merge_sort_impl(v, lo, mid);   // рекурсия на левой половине
    merge_sort_impl(v, mid, hi);   // рекурсия на правой половине
    merge(v, lo, mid, hi);         // слияние двух отсортированных половин
}

std::vector<int> merge_sort(std::vector<int> v) {
    if (v.size() > 1)
        merge_sort_impl(v, 0, v.size());
    return v;
}

// ── Задание 7. BST ─────────────────────────────────────────────────────────
// Это эталонный ответ (answer key) — не отгружается ученикам.

void BST::insert(int value) {
    // Хелпер по ссылке на «слот» родителя: вставка на пустом месте подвешивается сама.
    std::unique_ptr<Node>* slot = &root_;
    while (*slot) {
        if (value < (*slot)->value) {
            slot = &(*slot)->left;
        } else if (value > (*slot)->value) {
            slot = &(*slot)->right;
        } else {
            return;  // дубликат — ничего не делаем
        }
    }
    *slot = std::make_unique<Node>(value);
    ++size_;
}

bool BST::contains(int value) const {
    const Node* node = root_.get();
    while (node) {
        if (value < node->value) {
            node = node->left.get();
        } else if (value > node->value) {
            node = node->right.get();
        } else {
            return true;
        }
    }
    return false;
}

std::vector<int> BST::inorder() const {
    // In-order итеративно (явный стек указателей), чтобы не заводить внешний
    // хелпер по приватному Node. Порядок лево → себя → право даёт возрастание.
    std::vector<int> out;
    out.reserve(size_);
    std::vector<const Node*> stack;
    const Node* node = root_.get();
    while (node != nullptr || !stack.empty()) {
        while (node != nullptr) {       // спускаемся влево до упора
            stack.push_back(node);
            node = node->left.get();
        }
        node = stack.back();            // самый левый необработанный узел
        stack.pop_back();
        out.push_back(node->value);     // посещаем себя
        node = node->right.get();       // затем правое поддерево
    }
    return out;
}

std::size_t BST::size() const {
    return size_;
}
