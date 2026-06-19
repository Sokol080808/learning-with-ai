#include "kvstore.hpp"
// #include <fstream>   // для save/load

void KvStore::set(const std::string& key, const std::string& value) {
    // TODO (майлстоун 1)
    (void)key; (void)value;
}

std::optional<std::string> KvStore::get(const std::string& key) const {
    // TODO (майлстоун 1)
    (void)key;
    return std::nullopt;
}

bool KvStore::erase(const std::string& key) {
    // TODO (майлстоун 1)
    (void)key;
    return false;
}

std::size_t KvStore::size() const {
    // TODO (майлстоун 1)
    return 0;
}

std::vector<std::string> KvStore::keys() const {
    // TODO (майлстоун 1): отсортированные ключи
    return {};
}

bool KvStore::save(const std::string& path) const {
    // TODO (майлстоун 3): записать пары в файл
    (void)path;
    return false;
}

bool KvStore::load(const std::string& path) {
    // TODO (майлстоун 3): прочитать файл, заменить содержимое
    (void)path;
    return false;
}
