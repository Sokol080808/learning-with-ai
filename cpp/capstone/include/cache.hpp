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
//   - get(k):     найти узел по ключом (хеш) и ПЕРЕМЕСТИТЬ его в «голову» (most-recent);
//   - put(k, v):  обновить/вставить узел в голову; при переполнении удалить «хвост».
//
// TODO: реализуй Cache<K,V> — LRU-кэш фиксированной ёмкости на двусвязном списке
//       из unique_ptr-узлов + unordered_map<K, Node*> для O(1)-поиска.
//       Подсказки:
//         - Узел (Node) хранит key, value, сырой prev (не-владеющий) и unique_ptr next (владеющий).
//         - move_to_front(node*): вырезать узел из списка и переставить в голову.
//         - evict_lru(): удалить хвост (tail_) при переполнении.
//         - unlink_and_destroy(node*): вырезать узел, передав владение вызывающему коду.
//       Детали по unique_ptr-цепочке читай в README (модули 04/06).

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
    // capacity == 0 трактуем как «1»: кэш нулевой ёмкости бессмысленен.
    explicit Cache(std::size_t capacity)
        : capacity_(capacity == 0 ? 1 : capacity) {}

    // Запрещаем копирование (unique_ptr не копируется, сырые prev-указатели
    // ссылаются внутрь этого объекта).
    Cache(const Cache&) = delete;
    Cache& operator=(const Cache&) = delete;
    Cache(Cache&&) noexcept = default;
    Cache& operator=(Cache&&) noexcept = default;

    // Сколько элементов сейчас в кэше.
    std::size_t size() const {
        // TODO: вернуть реальный размер через index_.size()
        return 0;  // стаб
    }

    std::size_t capacity() const { return capacity_; }

    bool empty() const {
        // TODO: вернуть index_.empty()
        return true;  // стаб: всегда «пусто» — ломает PutGetBasic
    }

    // Есть ли ключ (БЕЗ влияния на порядок использования).
    bool contains(const K& key) const {
        // TODO: найти ключ в index_ и вернуть true/false
        (void)key;
        return false;  // стаб
    }

    // get(key): если ключ есть — пометить как most-recently-used и вернуть значение;
    // иначе std::nullopt.
    std::optional<V> get(const K& key) {
        // TODO: найти узел в index_, вызвать move_to_front, вернуть значение
        (void)key;
        return std::nullopt;  // стаб
    }

    // put(key, value): вставить или обновить. При переполнении вытеснить LRU.
    void put(const K& key, V value) {
        // TODO: обновить существующий или создать новый узел; при size >= capacity_ вызвать evict_lru()
        (void)key;
        (void)value;
        // стаб: ничего не сохраняет
    }

    // Удалить ключ. true — если он был.
    bool erase(const K& key) {
        // TODO: найти в index_, вырезать из списка, стереть из index_
        (void)key;
        return false;  // стаб
    }

    void clear() {
        // TODO: очистить index_, tail_ = nullptr, head_.reset()
        // стаб: ничего не делает
    }

private:
    // TODO: объяви структуру Node с полями key, value, prev (сырой), next (unique_ptr).
    //       Подробнее в комментарии к файлу выше.

    // TODO: реализуй move_to_front(Node*) — перецепить узел в голову.
    // TODO: реализуй evict_lru() — вытеснить хвост.
    // TODO: реализуй unlink_and_destroy(Node*) — вырезать и освободить узел.

    std::size_t capacity_;

    // TODO: добавь поля head_ (unique_ptr<Node>), tail_ (Node*), index_ (unordered_map<K, Node*>)
    //       и реализуй тела методов выше.
};

}  // namespace minidb
