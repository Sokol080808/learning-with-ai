// cache.hpp — СТРЕТЧ-ЦЕЛЬ капстоуна: header-only обобщённый LRU-кэш Cache<K, V>.
//
// Это НЕ обязательная часть MiniDB и НЕ часть его ядра — отдельный, самодостаточный
// заголовок, который закрывает две темы курса, заявленные в README, но не показанные
// «в полный рост» в основном коде:
//   1) ШАБЛОНЫ (модуль 07) — Cache параметризован типами ключа K и значения V;
//   2) УМНЫЕ УКАЗАТЕЛИ / RAII (модули 04/06) — узлы списка владеются через
//      std::unique_ptr, так что память освобождается автоматически, без delete.
//
// Что такое LRU. «Least Recently Used» — политика вытеснения: когда кэш переполнен,
// выкидываем тот элемент, к которому дольше всего не обращались. Идея в том, что
// «горячие» данные (часто читаемые) останутся, а «холодные» уйдут первыми.
//
// Классическая реализация = двусвязный список порядка использования + хеш-таблица
// «ключ → узел списка». Оба контейнера дают O(1) на основные операции:
//   - get(k):     найти узел по ключу (хеш) и ПЕРЕМЕСТИТЬ его в «голову» (most-recent);
//   - put(k, v):  обновить/вставить узел в голову; при переполнении удалить «хвост».
//
// ----------------------------------------------------------------------------------
// ПОЧЕМУ ЗДЕСЬ ИНТРУЗИВНЫЙ СПИСОК НА unique_ptr, А НЕ std::list.
//   std::list<...> + unordered_map<K, list::iterator> — самый короткий путь, и в
//   «боевом» коде он отличный. Но цель этого заголовка — ПОКАЗАТЬ владение через
//   unique_ptr своими руками: каждый узел владеет СЛЕДУЮЩИМ (next — unique_ptr), а
//   обратную связь (prev) держит «наблюдающим» сырым указателем (не-владеющим).
//   Так разрушение всего списка — это одно обнуление головы: unique_ptr цепочкой
//   рекурсивно (логически) освобождает все узлы. Ни одного delete в коде.
//
// КРАЕВОЙ СЛУЧАЙ ownership-дисциплины: prev обязан быть СЫРЫМ указателем. Если бы и
//   next, и prev были владеющими, получился бы цикл владения — деструкторы никогда
//   бы не отработали (висящая память). Правило: в двусвязной структуре владеет одно
//   направление, второе только наблюдает. (Альтернатива — shared_ptr + weak_ptr;
//   weak_ptr рвёт цикл, но добавляет атомарный счётчик ссылок — лишняя цена для
//   односложного владельца.)
// ----------------------------------------------------------------------------------

#pragma once

#include <cstddef>
#include <memory>
#include <optional>
#include <unordered_map>
#include <utility>

namespace minidb {

// LRU-кэш фиксированной ёмкости. K должен быть хешируемым (есть std::hash<K>).
template <typename K, typename V>
class Cache {
public:
    // capacity == 0 трактуем как «1»: кэш нулевой ёмкости бессмысленен и порождал бы
    // краевые случаи на каждом put. Минимально полезная ёмкость — один элемент.
    explicit Cache(std::size_t capacity)
        : capacity_(capacity == 0 ? 1 : capacity) {}

    // Запрещаем копирование: узлы владеются unique_ptr, а сырые prev-указатели
    // ссылаются внутрь ЭТОГО объекта. Дефолтный move корректен (переносит unique_ptr
    // и map; сырые prev остаются валидными, т.к. указывают на перемещённые узлы кучи).
    Cache(const Cache&) = delete;
    Cache& operator=(const Cache&) = delete;
    Cache(Cache&&) noexcept = default;
    Cache& operator=(Cache&&) noexcept = default;

    // Сколько элементов сейчас в кэше.
    std::size_t size() const { return index_.size(); }
    std::size_t capacity() const { return capacity_; }
    bool empty() const { return index_.empty(); }

    // Есть ли ключ (БЕЗ влияния на порядок использования — чисто наблюдение).
    bool contains(const K& key) const { return index_.find(key) != index_.end(); }

    // get(key): если ключ есть — пометить как most-recently-used и вернуть значение;
    // иначе std::nullopt. Возвращаем КОПИЮ значения, чтобы не отдавать наружу ссылку
    // на узел, который следующий put может вытеснить (висящая ссылка).
    std::optional<V> get(const K& key) {
        auto it = index_.find(key);
        if (it == index_.end()) return std::nullopt;
        Node* node = it->second;
        move_to_front(node);
        return node->value;
    }

    // put(key, value): вставить или обновить. Обновление существующего ключа НЕ меняет
    // размер, но делает его most-recent. Вставка нового при переполнении сперва
    // вытесняет least-recent (хвост). value берём по значению + move внутрь (модуль 05).
    void put(const K& key, V value) {
        auto it = index_.find(key);
        if (it != index_.end()) {  // обновление существующего
            Node* node = it->second;
            node->value = std::move(value);
            move_to_front(node);
            return;
        }
        if (index_.size() >= capacity_) evict_lru();  // освободить место под новый

        // Новый узел становится головой (most-recent). next пока пуст — нельзя
        // инициализировать его из head_ копией (unique_ptr не копируется): сначала
        // создаём узел, затем ПЕРЕМЕЩАЕМ в его next старую голову.
        auto node = std::make_unique<Node>();
        node->key = key;
        node->value = std::move(value);
        node->prev = nullptr;
        Node* raw = node.get();
        if (head_) head_->prev = raw;
        node->next = std::move(head_);  // node перенимает владение старой головой
        head_ = std::move(node);        // head_ теперь владеет node
        if (!tail_) tail_ = raw;        // первый элемент — он же и хвост
        index_.emplace(key, raw);
    }

    // Удалить ключ, если он есть. true — если что-то удалили.
    bool erase(const K& key) {
        auto it = index_.find(key);
        if (it == index_.end()) return false;
        unlink_and_destroy(it->second);
        index_.erase(it);
        return true;
    }

    void clear() {
        index_.clear();
        tail_ = nullptr;
        head_.reset();  // цепочка unique_ptr рекурсивно освобождает все узлы
    }

private:
    struct Node {
        K key;
        V value;
        Node* prev;                 // НЕ-владеющий: ближе к голове
        std::unique_ptr<Node> next; // ВЛАДЕЮЩИЙ: ближе к хвосту
    };

    // Перецепить node в голову списка (стал most-recently-used). Если уже голова —
    // ничего не делаем. Работаем с unique_ptr аккуратно: сначала «вынимаем» владение
    // узлом из его текущего места, потом вставляем в голову.
    void move_to_front(Node* node) {
        if (head_.get() == node) return;  // уже most-recent

        // 1) Извлечь владеющий unique_ptr на node из (node->prev->next).
        //    node->prev != nullptr, т.к. node не голова.
        std::unique_ptr<Node> owned = std::move(node->prev->next);
        // owned теперь единолично владеет node; node->prev->next пуст.

        // 2) Сшить соседей в обход node.
        node->prev->next = std::move(node->next);  // prev перенимает владение «правым» соседом
        if (node->prev->next) {
            node->prev->next->prev = node->prev;    // правый сосед смотрит на левого
        } else {
            tail_ = node->prev;  // node был хвостом → новый хвост = его prev
        }

        // 3) Вставить node в голову.
        node->prev = nullptr;
        node->next = std::move(head_);
        if (node->next) node->next->prev = node;
        head_ = std::move(owned);
    }

    // Вытеснить least-recently-used = хвост.
    void evict_lru() {
        if (!tail_) return;
        Node* victim = tail_;
        index_.erase(victim->key);
        unlink_and_destroy(victim);
    }

    // Вырезать узел из списка и освободить его (unique_ptr сделает delete сам).
    void unlink_and_destroy(Node* node) {
        Node* prev = node->prev;
        if (prev) {
            // node не голова: владелец node — это prev->next.
            if (node->next) node->next->prev = prev;
            else            tail_ = prev;
            prev->next = std::move(node->next);  // освобождает старый node
        } else {
            // node — голова: владелец — head_.
            if (node->next) node->next->prev = nullptr;
            else            tail_ = nullptr;
            head_ = std::move(node->next);       // освобождает старую голову
        }
    }

    std::size_t capacity_;
    std::unique_ptr<Node> head_{};            // most-recently-used (владеет цепочкой)
    Node* tail_{nullptr};                     // least-recently-used (наблюдающий)
    std::unordered_map<K, Node*> index_{};    // ключ → узел (наблюдающие указатели)
};

}  // namespace minidb
