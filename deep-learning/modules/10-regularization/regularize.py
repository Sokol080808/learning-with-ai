# ВНИМАНИЕ: здесь пишешь ТЫ. Реализуй так, чтобы тесты модуля 10 стали зелёными.
#
# Стек модуля: ТОЛЬКО numpy (никакого torch). Всё — руками.
# Тело каждой функции пока бросает NotImplementedError. Замени его своим кодом.

from typing import Tuple

import numpy as np


def train_val_split(
    X: np.ndarray,
    y: np.ndarray,
    val_frac: float,
    seed: int,
) -> Tuple[np.ndarray, np.ndarray, np.ndarray, np.ndarray]:
    """Детерминированно перемешать данные и отрезать валидационную часть.

    Контракт:
      - X: (N, D) — признаки, y: (N,) или (N, K) — цели; первая ось у обоих равна N.
      - Перемешай ИНДЕКСЫ строк через np.random.default_rng(seed).permutation(N)
        (одна и та же перестановка для X и y — пары «признаки↔цель» не должны разъехаться).
      - Размер валидации: n_val = int(round(N * val_frac)). Первые n_val строк
        перемешанного массива идут в валидацию, остальные — в обучение.
      - Верни кортеж (X_tr, y_tr, X_val, y_val).
        Формы: X_tr (N - n_val, D), X_val (n_val, D); y_tr/y_val — со своей второй осью.
      - При одном и том же seed результат обязан быть БИТ-В-БИТ одинаковым.

    Зачем: модель оценивают на данных, которых она НЕ видела при обучении. Если мерить
    качество на тех же примерах, что учили, — обманешь сам себя (см. README, Идея 2).
    """
    N = X.shape[0]
    rng = np.random.default_rng(seed)
    idx = rng.permutation(N)
    n_val = int(round(N * val_frac))
    val_idx = idx[:n_val]
    tr_idx = idx[n_val:]
    return X[tr_idx], y[tr_idx], X[val_idx], y[val_idx]


def l2_penalty(weights: np.ndarray, lam: float) -> float:
    """L2-штраф за большие веса: lam * сумма квадратов всех элементов weights.

    Контракт:
      - weights: массив любой формы (например (D,) или (D, K)).
      - Верни ПИТОНОВСКИЙ float, равный lam * np.sum(weights ** 2).
      - lam >= 0. При lam == 0 штраф равен 0.0 при любых весах.

    Зачем: добавляя этот штраф к loss, мы «наказываем» модель за крупные веса и
    подталкиваем её к более гладким, простым решениям (см. README, Идея 3).
    """
    return float(lam * np.sum(weights ** 2))


def dropout_mask(shape: Tuple[int, ...], p: float, seed: int) -> np.ndarray:
    """Маска дропаута: каждый нейрон с вероятностью p «гасится» (умножается на 0).

    Контракт:
      - shape: форма маски, например (N, H).
      - p: вероятность ОБНУЛИТЬ элемент (0 <= p < 1). p == 0 → маска вся из единиц.
      - Сгенерируй rng = np.random.default_rng(seed). Возьми
        keep = (rng.random(shape) >= p)  # True там, где нейрон оставляем
        и верни маску float, где оставленный элемент равен 1/(1-p), а погашенный — 0.0.
      - Форма результата совпадает с shape; dtype — float.

    Масштаб 1/(1-p) — это «inverted dropout»: он держит средний масштаб активаций
    неизменным, поэтому на инференсе маску можно просто не применять (см. README, Идея 4).
    """
    rng = np.random.default_rng(seed)
    keep = rng.random(shape) >= p
    return keep.astype(float) / (1.0 - p)


def should_early_stop(val_losses: list, patience: int) -> bool:
    """Решить, пора ли останавливать обучение (ранняя остановка).

    Контракт:
      - val_losses: список значений валидационного loss по эпохам (val_losses[-1] — последняя).
      - patience: сколько эпох подряд БЕЗ улучшения мы готовы терпеть (patience >= 1).
      - «Улучшение» = новый минимум: значение строго меньше всех предыдущих.
      - Верни True, если за последние `patience` эпох не было ни одного нового минимума,
        то есть лучший loss был достигнут как минимум `patience` эпох назад.
      - Пока эпох меньше, чем patience + 1, останавливаться рано → верни False.

    Пример: val_losses = [1.0, 0.8, 0.9, 0.95], patience = 2.
      Лучший (0.8) — на индексе 1. После него прошло 2 эпохи без улучшения → True.
    """
    if len(val_losses) < patience + 1:
        return False
    best = int(np.argmin(val_losses))
    epochs_without_improvement = len(val_losses) - 1 - best
    return epochs_without_improvement >= patience


def train_regularized(
    X_tr: np.ndarray,
    y_tr: np.ndarray,
    X_val: np.ndarray,
    y_val: np.ndarray,
    w_init: np.ndarray,
    lam: float,
    dropout_p: float,
    lr: float,
    patience: int,
    max_epochs: int,
) -> Tuple[np.ndarray, list]:
    """Мини-цикл обучения линейной регрессии со всеми регуляризаторами модуля сразу.

    Это «сборка» четырёх кирпичиков (train/val-сплит уже сделан снаружи, а тут — L2,
    dropout и ранняя остановка) в один настоящий обучающий цикл. На каждой эпохе:

      1. TRAIN-ВЕТКА. Строим dropout-маску формой X_tr через dropout_mask(X_tr.shape,
         dropout_p, seed=epoch) и поэлементно умножаем признаки: X_drop = X_tr * mask.
         Маска инвертированная (масштаб 1/(1-p)) — средний масштаб признаков сохраняется.
         Сид маски равен номеру эпохи, поэтому при каждом запуске цикл детерминирован,
         но по эпохам маска разная (как и положено dropout).
      2. ГРАДИЕНТНЫЙ ШАГ. Предсказание pred = X_drop @ w, ошибка err = pred - y_tr.
         Градиент MSE по w: (2/N) * X_drop.T @ err. Добавляем производную L2-штрафа
         lam*Σwᵢ² по w, равную 2*lam*w (см. README, Идея 3). Шаг: w = w - lr * grad.
      3. EVAL-ВЕТКА. Маску НЕ применяем (inverted dropout: на инференсе масштаб уже
         согласован). Считаем val_mse = mean((X_val @ w - y_val)**2), кладём в val_losses.
      4. ЧЕКПОИНТ + РАННЯЯ ОСТАНОВКА. Если это новый минимум val — запоминаем w как w_best.
         Затем спрашиваем should_early_stop(val_losses, patience): как только True — выходим
         из цикла досрочно, не доходя до max_epochs.

    Контракт:
      - X_tr (N_tr, D), y_tr (N_tr,), X_val (N_val, D), y_val (N_val,); w_init (D,).
      - lam >= 0, 0 <= dropout_p < 1, lr > 0, patience >= 1, max_epochs >= 1.
      - Веса НЕ портим in-place: стартуем с w = w_init.astype(float).copy().
      - Возвращаем (w_best, val_losses): w_best — веса лучшей по val эпохи (а не последней!),
        val_losses — список питоновских float, по одному на каждую пройденную эпоху.
        len(val_losses) <= max_epochs (ранняя остановка может оборвать раньше).
      - При dropout_p == 0 маска — все единицы, train-ветка == обычный градиентный спуск
        на полном X_tr. При lam == 0 L2-член градиента равен нулю. Значит lam=0, dropout_p=0
        воспроизводит ручной градиентный спуск бит-в-бит (с точностью до float).

    Зачем: в модулях 1–4 ты собрал инструменты по отдельности. Реальное обучение — это их
    связка в одном цикле с переключением train/eval-режима для dropout. Именно это и делает
    эта функция (ср. CS231n Solver с dropout, тетрадки d2l 4.5–4.6).
    """
    w = w_init.astype(float).copy()
    val_losses: list = []
    w_best = w.copy()
    best_val = float("inf")

    for epoch in range(max_epochs):
        # --- train branch: dropout активен ---
        mask = dropout_mask(X_tr.shape, dropout_p, seed=epoch)
        X_drop = X_tr * mask
        pred = X_drop @ w
        err = pred - y_tr
        grad = (2.0 / len(y_tr)) * (X_drop.T @ err) + 2.0 * lam * w
        w = w - lr * grad

        # --- eval branch: маску НЕ применяем ---
        val_mse = float(np.mean((X_val @ w - y_val) ** 2))
        val_losses.append(val_mse)

        # --- чекпоинт лучшей по val точки ---
        if val_mse < best_val:
            best_val = val_mse
            w_best = w.copy()

        # --- ранняя остановка ---
        if should_early_stop(val_losses, patience):
            break

    return w_best, val_losses


def confusion_counts(y_true: np.ndarray, y_pred: np.ndarray) -> Tuple[int, int, int, int]:
    """Посчитать ячейки матрицы ошибок для бинарных меток 0/1.

    Контракт:
      - y_true, y_pred: массивы формы (N,) из 0 и 1 (уже «жёсткие» предсказания — то есть
        порог к вероятностям применён где-то раньше, ср. accuracy(p, y, thr) из модуля 05).
      - Верни кортеж ПИТОНОВСКИХ int (tp, fp, fn, tn):
          tp — истинно-положительные: y_true == 1 И y_pred == 1
          fp — ложно-положительные:  y_true == 0 И y_pred == 1 («ложная тревога»)
          fn — ложно-отрицательные:  y_true == 1 И y_pred == 0 («пропуск»)
          tn — истинно-отрицательные: y_true == 0 И y_pred == 0
      - Всегда tp + fp + fn + tn == N (каждый пример попадает ровно в одну ячейку).

    Зачем: матрица ошибок — это «исходный объект», из которого выводятся precision/recall/F1
    (см. README, Идея 5). Accuracy скрывает, КАКИЕ именно ошибки делает модель; матрица — нет.
    """
    yt = np.asarray(y_true)
    yp = np.asarray(y_pred)
    tp = int(((yt == 1) & (yp == 1)).sum())
    fp = int(((yt == 0) & (yp == 1)).sum())
    fn = int(((yt == 1) & (yp == 0)).sum())
    tn = int(((yt == 0) & (yp == 0)).sum())
    return tp, fp, fn, tn


def precision_recall_f1(y_true: np.ndarray, y_pred: np.ndarray) -> Tuple[float, float, float]:
    """Precision, recall и F1 для бинарной классификации (через confusion_counts).

    Контракт:
      - y_true, y_pred: (N,) массивы из 0/1 (жёсткие предсказания).
      - precision = TP / (TP + FP) — «когда модель сказала 1, насколько ей верить».
      - recall    = TP / (TP + FN) — «какую долю настоящих единиц модель поймала».
      - F1        = 2*P*R / (P + R) — гармоническое среднее P и R.
      - Вырожденные случаи фиксируем явно (чтобы тесты их пинали):
          precision = 0.0, если TP + FP == 0 (модель не предсказала ни одной 1);
          recall    = 0.0, если TP + FN == 0 (в данных не было ни одной настоящей 1);
          F1        = 0.0, если P + R == 0.
      - Возвращаем кортеж ПИТОНОВСКИХ float (P, R, F1).

    Зачем: на несбалансированных данных accuracy врёт (модель «всегда 0» даёт 95% точности
    при 5% положительных, но recall = 0 — она не поймала НИ ОДНОГО важного случая). P/R/F1
    разбирают качество по «положительному» классу честно (см. README, Идея 5).
    """
    tp, fp, fn, tn = confusion_counts(y_true, y_pred)
    precision = tp / (tp + fp) if (tp + fp) > 0 else 0.0
    recall = tp / (tp + fn) if (tp + fn) > 0 else 0.0
    f1 = 2.0 * precision * recall / (precision + recall) if (precision + recall) > 0 else 0.0
    return float(precision), float(recall), float(f1)
