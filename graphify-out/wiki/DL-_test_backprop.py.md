# DL: test_backprop.py

> 32 nodes · cohesion 0.09

## Key Concepts

- **test_backprop.py** (16 connections) — `deep-learning/modules/07-backprop/test_backprop.py`
- **make_net()** (7 connections) — `deep-learning/modules/07-backprop/test_backprop.py`
- **backprop.py** (6 connections) — `deep-learning/modules/07-backprop/backprop.py`
- **backward2()** (6 connections) — `deep-learning/modules/07-backprop/backprop.py`
- **ndarray** (6 connections) — `deep-learning/modules/07-backprop/backprop.py`
- **test_backward2_softmax_ce_gradient_on_logits_is_p_minus_y()** (4 connections) — `deep-learning/modules/07-backprop/test_backprop.py`
- **cross_entropy()** (3 connections) — `deep-learning/modules/07-backprop/backprop.py`
- **forward2()** (3 connections) — `deep-learning/modules/07-backprop/backprop.py`
- **numerical_gradient()** (3 connections) — `deep-learning/modules/07-backprop/backprop.py`
- **relu()** (3 connections) — `deep-learning/modules/07-backprop/backprop.py`
- **softmax()** (3 connections) — `deep-learning/modules/07-backprop/backprop.py`
- **test_backward2_matches_numerical_gradient()** (3 connections) — `deep-learning/modules/07-backprop/test_backprop.py`
- **test_training_with_backward2_reduces_loss()** (3 connections) — `deep-learning/modules/07-backprop/test_backprop.py`
- **onehot()** (2 connections) — `deep-learning/modules/07-backprop/test_backprop.py`
- **test_forward2_matches_manual_composition()** (2 connections) — `deep-learning/modules/07-backprop/test_backprop.py`
- **test_forward2_returns_scalar_loss()** (2 connections) — `deep-learning/modules/07-backprop/test_backprop.py`
- **Численный градиент скалярной функции f по массиву param (центральная разность).** (1 connections) — `deep-learning/modules/07-backprop/backprop.py`
- **ReLU поэлементно: max(0, z). Форма сохраняется.      Контракт: возвращает массив** (1 connections) — `deep-learning/modules/07-backprop/backprop.py`
- **Softmax по строкам (по последней оси, axis=-1).      Вход z: (N, C) — логиты. Вы** (1 connections) — `deep-learning/modules/07-backprop/backprop.py`
- **Средняя кросс-энтропия по батчу.      p: (N, C) — вероятности (выход softmax). y** (1 connections) — `deep-learning/modules/07-backprop/backprop.py`
- **Прямой проход двухслойной сети до значения потерь.      Считает: z1 = x@W1+b1 ->** (1 connections) — `deep-learning/modules/07-backprop/backprop.py`
- **Обратное распространение для двухслойной сети (relu + softmax + cross-entropy).** (1 connections) — `deep-learning/modules/07-backprop/backprop.py`
- **Возвращает (x, y, W1, b1, W2, b2) — крошечная двухслойная сеть и батч с метками.** (1 connections) — `deep-learning/modules/07-backprop/test_backprop.py`
- **test_cross_entropy_confident_correct_is_small()** (1 connections) — `deep-learning/modules/07-backprop/test_backprop.py`
- **test_cross_entropy_known_value()** (1 connections) — `deep-learning/modules/07-backprop/test_backprop.py`
- *... and 7 more nodes in this community*

## Relationships

- No strong cross-community connections detected

## Source Files

- `deep-learning/modules/07-backprop/backprop.py`
- `deep-learning/modules/07-backprop/test_backprop.py`

## Audit Trail

- EXTRACTED: 82 (93%)
- INFERRED: 6 (7%)
- AMBIGUOUS: 0 (0%)

---

*Part of the graphify knowledge wiki. See [[index]] to navigate.*