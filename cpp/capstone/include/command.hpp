#pragma once
#include <string>
#include "kvstore.hpp"

// Разобрать одну строку-команду, выполнить её над store и вернуть текстовый ответ.
// Поддерживаемые команды и ответы описаны в README (майлстоун 2 и 3).
std::string execute(KvStore& store, const std::string& line);
