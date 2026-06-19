#pragma once
#include <memory>
#include <vector>
#include <cstddef>

// Вернуть unique_ptr, владеющий int со значением v.
std::unique_ptr<int> make_unique_int(int v);

// Односвязный список, владеющий узлами через unique_ptr (правило нуля в действии).
class IntList {
public:
    void push_front(int value);
    std::size_t size() const;
    bool contains(int value) const;
    std::vector<int> to_vector() const;  // от головы к хвосту

private:
    struct Node {
        int value;
        std::unique_ptr<Node> next;
    };
    std::unique_ptr<Node> head_;
};
