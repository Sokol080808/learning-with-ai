#pragma once
#include <vector>
#include <map>
#include <string>
#include <cstddef>

// Только чётные элементы, в исходном порядке.
std::vector<int> evens(const std::vector<int>& v);

// Сколько элементов строго больше threshold.
int count_greater(const std::vector<int>& v, int threshold);

// Каждый элемент возведён в квадрат.
std::vector<int> squared(const std::vector<int>& v);

// Копия v, отсортированная по убыванию.
std::vector<int> sorted_desc(std::vector<int> v);

// Частота каждого символа строки.
std::map<char, int> char_frequency(const std::string& s);

// --- Дополнительные (более сложные) задания ---

// Частотный словарь слов. Текст разбивается по пробельным символам (через istringstream),
// каждое слово приводится к нижнему регистру (только латиница a-z, остальные символы
// не трогаем). Возвращает map: слово -> сколько раз встретилось.
std::map<std::string, int> word_frequency(const std::string& text);

// k наибольших элементов вектора, отсортированных по убыванию.
// Если k <= 0 -> пустой вектор. Если k >= v.size() -> все элементы по убыванию.
// Исходный вектор передаётся по значению — можно переупорядочивать копию.
// Подсказка: std::nth_element / std::partial_sort.
std::vector<int> top_k(std::vector<int> v, int k);

// Удаляет из вектора (на месте, изменяя аргумент по ссылке) все элементы, строго
// меньшие threshold. Возвращает количество УДАЛЁННЫХ элементов.
// Подсказка: идиома erase-remove (std::remove_if + v.erase).
std::size_t drop_below(std::vector<int>& v, int threshold);

// --- Задание на C++20 Ranges (Идея 8) ---

// Возвращает квадраты чётных элементов xs в исходном порядке.
// Реализовать ЧЕРЕЗ ОДНУ ranges-цепочку:
//   xs | views::filter(чётность) | views::transform(квадрат)
// и материализовать результат в вектор через std::ranges::copy + back_inserter.
// Промежуточные std::vector создавать НЕЛЬЗЯ.
// Пример: evens_squared({1,2,3,4,5,6}) -> {4,16,36}
std::vector<int> evens_squared(const std::vector<int>& xs);
