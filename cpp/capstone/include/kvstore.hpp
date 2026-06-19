#pragma once
#include <string>
#include <optional>
#include <vector>
#include <map>
#include <cstddef>

// Хранилище «ключ -> значение» со строковыми ключами и значениями.
class KvStore {
public:
    // Вставить или перезаписать значение по ключу.
    void set(const std::string& key, const std::string& value);

    // Значение по ключу или std::nullopt, если ключа нет.
    std::optional<std::string> get(const std::string& key) const;

    // Удалить ключ. true, если ключ существовал.
    bool erase(const std::string& key);

    // Количество записей.
    std::size_t size() const;

    // Все ключи в отсортированном порядке.
    std::vector<std::string> keys() const;

    // Сохранить всё содержимое в файл. true при успехе.
    bool save(const std::string& path) const;

    // Загрузить содержимое из файла, ЗАМЕНИВ текущее. false, если файл не открылся.
    bool load(const std::string& path);

private:
    std::map<std::string, std::string> data_;
};
