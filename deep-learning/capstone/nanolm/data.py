# ВНИМАНИЕ: здесь пишешь ТЫ. Реализуй так, чтобы приёмочные тесты капстоуна стали зелёными.
#
# nanoLM — нарезка обучающих батчей для языковой модели.
# Языковая модель учится предсказывать СЛЕДУЮЩИЙ символ. Значит, обучающий пример —
# это пара (контекст x, ответ y), где y — это x, сдвинутый ровно на один символ
# вперёд. Здесь ты реализуешь функцию, которая вырезает такие пары из длинного
# текста (уже закодированного в индексы).

from typing import Tuple

import torch


def get_batch(
    data: torch.Tensor,
    block_size: int,
    batch_size: int,
    seed: int,
) -> Tuple[torch.Tensor, torch.Tensor]:
    """Нарезать случайный батч обучающих пар (x, y) из последовательности индексов.

    Вход:
      - data: 1-D LongTensor индексов символов, длина L (это encode всего корпуса,
        обёрнутый в torch.tensor(..., dtype=torch.long));
      - block_size: длина контекста T (сколько символов видит модель за раз);
      - batch_size: сколько примеров в батче B;
      - seed: фиксирует случайный выбор стартовых позиций -> батч воспроизводим.

    Что делаем. Выбираем B случайных стартовых позиций i так, чтобы из data можно
    было взять кусок длиной block_size И ещё один символ для сдвига. То есть i
    лежит в диапазоне [0, L - block_size - 1] включительно. Для каждой позиции i:
        x = data[i : i + block_size]            # контекст
        y = data[i + 1 : i + block_size + 1]    # тот же кусок, сдвинутый на 1
    Складываем B таких кусков в батч.

    Возвращает (x, y):
      - x: LongTensor формы (batch_size, block_size);
      - y: LongTensor формы (batch_size, block_size);
      - инвариант сдвига: y[b, t] == x[b, t + 1] для всех t < block_size - 1,
        а y[b, block_size - 1] — это символ, идущий сразу ЗА куском x[b].
        Иначе говоря, для позиции t в x правильный «следующий символ» — это y[b, t].

    Детерминизм. Перед выбором позиций зафиксируй генератор по seed (например
    torch.Generator().manual_seed(seed) и используй его в torch.randint(...,
    generator=g)), чтобы при одном seed батч получался один и тот же.

    Пример формы. data длины 100, block_size=8, batch_size=4 ->
    x: (4, 8), y: (4, 8); каждая строка y — соответствующая строка x, сдвинутая
    на один символ.
    """
    g = torch.Generator().manual_seed(seed)
    # Стартовые позиции i: из data[i:i+block_size] И ещё один символ для сдвига.
    # Допустимый диапазон i — [0, L - block_size - 1], то есть randint с верхней
    # (исключительной) границей L - block_size.
    ix = torch.randint(0, len(data) - block_size, (batch_size,), generator=g)
    x = torch.stack([data[i : i + block_size] for i in ix])
    y = torch.stack([data[i + 1 : i + block_size + 1] for i in ix])
    return x, y
