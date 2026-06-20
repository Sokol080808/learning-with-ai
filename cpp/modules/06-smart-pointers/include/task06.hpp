#pragma once
#include <memory>
#include <vector>
#include <string>
#include <cstddef>
#include <optional>

// Вернуть unique_ptr, владеющий int со значением v.
std::unique_ptr<int> make_unique_int(int v);

// Задание 6. Безопасный доступ к элементу вектора через optional.
// Возвращает v[i], если i < v.size(); иначе std::nullopt.
// Иллюстрирует std::optional как nullable value без кучи (Идея 11).
std::optional<int> maybe_at(const std::vector<int>& v, std::size_t i);

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

// ---------------------------------------------------------------------------
// Задание 3. Дерево: владение детьми через shared_ptr, ссылка на родителя — weak_ptr.
//
// Узел владеет своими детьми (shared_ptr), но НЕ владеет родителем: если бы parent был
// shared_ptr, образовался бы цикл (родитель держит ребёнка, ребёнок держит родителя),
// и счётчики ссылок никогда не упали бы до нуля — утечка. Поэтому parent — это weak_ptr.
//
// TreeNode нельзя конструировать «голым» new и класть в shared_ptr вручную: чтобы
// корректно завести weak_ptr на самого себя при добавлении ребёнка, узел должен уметь
// получить shared_ptr на себя. Для этого наследуем std::enable_shared_from_this.
class TreeNode : public std::enable_shared_from_this<TreeNode> {
public:
    explicit TreeNode(int value) : value_(value) {}

    int value() const { return value_; }

    // Создать узел-ребёнка со значением child_value, прицепить его к this как ребёнка,
    // выставить ему parent (weak_ptr на this) и вернуть shared_ptr на нового ребёнка.
    std::shared_ptr<TreeNode> add_child(int child_value);

    // Сколько прямых детей.
    std::size_t child_count() const;

    // Доступ к i-му ребёнку (для тестов). Без проверки границ — тест передаёт валидный i.
    std::shared_ptr<TreeNode> child(std::size_t i) const;

    // Вернуть shared_ptr на родителя, повысив weak_ptr через lock().
    // Для корня (родителя нет) вернуть nullptr.
    std::shared_ptr<TreeNode> parent() const;

    // Сумма value() этого узла и всех потомков (рекурсивно по поддереву).
    int subtree_sum() const;

private:
    int value_;
    std::vector<std::shared_ptr<TreeNode>> children_;
    std::weak_ptr<TreeNode> parent_;
};

// Удобная фабрика корня дерева.
std::shared_ptr<TreeNode> make_tree_root(int value);

// ---------------------------------------------------------------------------
// Задание 4. Pimpl на unique_ptr (Pointer to IMPLementation).
//
// Публичный класс Counter скрывает своё устройство за указателем на незавершённый тип
// Impl. Объявление Impl есть только в .cpp — заголовок про его поля ничего не знает.
// Это разрывает зависимость по компиляции: меняя приватные поля, не приходится
// перекомпилировать всех, кто включает этот заголовок.
//
// Тонкость: unique_ptr<Impl> в деструкторе должен «знать» полный тип Impl, чтобы его
// удалить. Поэтому деструктор НЕЛЬЗЯ оставить неявным в заголовке — его надо объявить
// здесь и определить в .cpp (после полного определения Impl). То же касается операций
// перемещения, если они нужны.
class Counter {
public:
    explicit Counter(int start = 0);
    ~Counter();                                   // объявлен здесь, определён в .cpp

    Counter(Counter&&) noexcept;                  // перемещение разрешено
    Counter& operator=(Counter&&) noexcept;

    Counter(const Counter&) = delete;             // копирование запрещаем (для простоты)
    Counter& operator=(const Counter&) = delete;

    void increment();          // +1
    void add(int delta);       // +delta
    int value() const;         // текущее значение

private:
    struct Impl;                       // незавершённый тип — определён в .cpp
    std::unique_ptr<Impl> impl_;
};

// ---------------------------------------------------------------------------
// Задание 5. Общий ресурс и use_count при копировании shared_ptr.
//
// SharedResource — обёртка над shared_ptr<std::string>. Цель — на практике увидеть,
// как копирование shared_ptr меняет счётчик владельцев, а его падение до нуля
// освобождает ресурс.
class SharedResource {
public:
    // Создать ресурс, владеющий строкой data (через make_shared). use_count() == 1.
    explicit SharedResource(std::string data);

    // Текущее число владельцев общего блока (delegates to shared_ptr::use_count()).
    long use_count() const;

    // Прочитать содержимое (по значению, чтобы тест не зависел от живости *this).
    std::string read() const;

    // Создать ещё одного владельца ТОГО ЖЕ ресурса (копия shared_ptr внутри).
    // После вызова use_count() обоих объектов вырастет на 1.
    SharedResource share() const;

private:
    std::shared_ptr<std::string> data_;
};
