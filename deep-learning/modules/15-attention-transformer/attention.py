# ВНИМАНИЕ: здесь пишешь ТЫ. Реализуй так, чтобы тесты модуля 15 стали зелёными.
#
# Модуль 15 — Внимание и трансформеры.
# Здесь ты своими руками собираешь сердце современных языковых моделей:
# scaled dot-product attention, причинную маску, многоголовое внимание и
# блок трансформера. Никаких готовых nn.MultiheadAttention — только базовые
# кирпичики torch, чтобы ты понимал КАЖДУЮ строчку.
# Весь модуль — на CPU, всё во float32.

import math
from typing import Optional, Tuple

import torch
import torch.nn as nn


def scaled_dot_product_attention(
    q: torch.Tensor,
    k: torch.Tensor,
    v: torch.Tensor,
    mask: Optional[torch.Tensor] = None,
) -> Tuple[torch.Tensor, torch.Tensor]:
    """Scaled dot-product attention — базовая операция внимания.

    Идея: каждая позиция-запрос (query) смотрит на все позиции-ключи (key),
    считает «насколько они похожи», превращает похожести в веса (softmax) и
    собирает взвешенную смесь значений (value).

    Формы (d_k — размер ключа/запроса, d_v — размер значения):
      - q: (..., T_q, d_k)   — T_q запросов;
      - k: (..., T_k, d_k)   — T_k ключей;
      - v: (..., T_k, d_v)   — T_k значений (столько же, сколько ключей);
      - mask: (..., T_q, T_k) или (T_q, T_k), либо None.
        Где mask == True — позицию ЗАПРЕЩАЕМ (её вес станет ~0).

    Возвращает кортеж (out, attn_weights):
      - out:          (..., T_q, d_v)  — результат внимания;
      - attn_weights: (..., T_q, T_k)  — веса после softmax (каждая строка
                                         суммируется в 1).

    Шаги, которые нужно реализовать:
      1) scores = q @ kᵀ / sqrt(d_k)         # форма (..., T_q, T_k)
      2) если mask задан — в запрещённых позициях поставь -inf
         (например scores.masked_fill(mask, float('-inf')))
      3) attn = softmax(scores) по последней оси (по ключам, dim=-1)
      4) out = attn @ v
      5) верни (out, attn)

    ПОЧЕМУ делим на sqrt(d_k): при больших d_k скалярные произведения растут
    по модулю, softmax «насыщается» (почти one-hot) и градиенты гаснут.
    Деление на sqrt(d_k) держит масштаб scores стабильным.

    Маленький пример формы: q,k,v: (1, 3, 4) -> out: (1, 3, 4), attn: (1, 3, 3).
    """
    d_k = q.shape[-1]
    scores = (q @ k.transpose(-2, -1)) / math.sqrt(d_k)
    if mask is not None:
        scores = scores.masked_fill(mask, float("-inf"))
    attn = torch.softmax(scores, dim=-1)
    out = attn @ v
    return out, attn


def causal_mask(T: int) -> torch.Tensor:
    """Причинная (causal) маска: запрет «подглядывать в будущее».

    Контракт:
      - вернуть булев тензор формы (T, T);
      - элемент [i, j] == True означает «позиции i ЗАПРЕЩЕНО смотреть на j»;
      - запрещаем все будущие позиции: j > i (строго выше главной диагонали);
      - позицию i и все прошлые (j <= i) разрешаем (== False).

    Пример для T = 3 (True = запрет):
        [[False,  True,  True],
         [False, False,  True],
         [False, False, False]]
      Строка 0 видит только себя; строка 2 видит всех.

    ПОЧЕМУ так: в авторегрессионной генерации токен предсказывается ТОЛЬКО по
    прошлому. Если позволить смотреть вперёд — модель «жульничает», подсматривая
    ответ, и на инференсе (где будущего ещё нет) развалится.

    Подсказка: пригодится torch.triu(..., diagonal=1) над тензором из единиц,
    приведённый к bool. Маска должна состыковываться с scaled_dot_product_attention,
    где True = запрет.
    """
    return torch.triu(torch.ones(T, T), diagonal=1).bool()


class MultiHeadAttention(nn.Module):
    """Многоголовое внимание (self-attention) с причинной маской.

    Идея: вместо одного внимания на весь d_model запускаем n_heads «голов»,
    каждая работает в подпространстве размером d_head = d_model // n_heads.
    Разные головы учатся смотреть на разные связи; их выходы склеиваются обратно
    в d_model и пропускаются через выходную проекцию.

    __init__(d_model, n_heads):
      - сохрани d_model, n_heads, посчитай d_head = d_model // n_heads
        (требуй, чтобы d_model делился на n_heads нацело);
      - создай проекции Q, K, V и выходную: четыре nn.Linear(d_model, d_model).
        (Удобно держать их раздельными: self.w_q, self.w_k, self.w_v, self.w_o.)

    forward(x) -> (B, T, d_model):
      Вход  x: (B, T, d_model). Шаги:
      1) посчитай q = w_q(x), k = w_k(x), v = w_v(x)         # все (B, T, d_model)
      2) «разрежь» последнюю ось на головы и переставь оси так, чтобы получить
         форму (B, n_heads, T, d_head). Приём:
         x.view(B, T, n_heads, d_head).transpose(1, 2)
      3) построй причинную маску causal_mask(T) и приведи её к устройству x;
         в scaled_dot_product_attention она броадкастится на оси (B, n_heads).
      4) примени scaled_dot_product_attention(q, k, v, mask) -> out (B, n_heads, T, d_head)
      5) «склей» головы обратно: transpose(1, 2) и reshape в (B, T, d_model)
      6) верни w_o(склеенное)                                  # (B, T, d_model)

    ПОЧЕМУ маска именно причинная: это self-attention для авторегрессии —
    позиция t не должна видеть t+1, t+2, …  Маска (T, T) одинакова для всех
    голов и всех объектов батча, поэтому строим её один раз.

    Форма выхода всегда совпадает с формой входа: (B, T, d_model) -> (B, T, d_model).
    """

    def __init__(self, d_model: int, n_heads: int) -> None:
        super().__init__()
        assert d_model % n_heads == 0, "d_model должен делиться на n_heads нацело"
        self.d_model = d_model
        self.n_heads = n_heads
        self.d_head = d_model // n_heads
        self.w_q = nn.Linear(d_model, d_model)
        self.w_k = nn.Linear(d_model, d_model)
        self.w_v = nn.Linear(d_model, d_model)
        self.w_o = nn.Linear(d_model, d_model)

    def forward(self, x: torch.Tensor) -> torch.Tensor:
        B, T, _ = x.shape

        # Проекции и резка на головы -> (B, n_heads, T, d_head).
        q = self.w_q(x).view(B, T, self.n_heads, self.d_head).transpose(1, 2)
        k = self.w_k(x).view(B, T, self.n_heads, self.d_head).transpose(1, 2)
        v = self.w_v(x).view(B, T, self.n_heads, self.d_head).transpose(1, 2)

        # Причинная маска (T, T) броадкастится на оси (B, n_heads).
        mask = causal_mask(T).to(x.device)
        out, _ = scaled_dot_product_attention(q, k, v, mask)

        # Склейка голов обратно -> (B, T, d_model) и выходная проекция.
        out = out.transpose(1, 2).reshape(B, T, self.d_model)
        return self.w_o(out)


class TransformerBlock(nn.Module):
    """Блок трансформера: pre-LayerNorm + (MHA & residual) + (MLP & residual).

    Это «кирпич», из которого стопкой собирают GPT-подобную сеть. Схема
    (pre-norm, как в современных моделях):

        x = x + MultiHeadAttention(LayerNorm1(x))
        x = x + MLP(LayerNorm2(x))

    __init__(d_model, n_heads, d_ff=None):
      - self.ln1, self.ln2 = nn.LayerNorm(d_model);
      - self.attn = MultiHeadAttention(d_model, n_heads);
      - MLP: nn.Linear(d_model, d_ff) -> nn.GELU() -> nn.Linear(d_ff, d_model),
        где d_ff по умолчанию 4 * d_model (классический множитель).

    forward(x) -> (B, T, d_model):
      1) x = x + self.attn(self.ln1(x))     # внимание + остаточная связь
      2) x = x + self.mlp(self.ln2(x))      # MLP + остаточная связь
      3) верни x

    ПОЧЕМУ residual (x + ...): остаточные связи дают градиенту «короткий путь»
    в обход блока, поэтому глубокие стопки блоков обучаются стабильно.
    ПОЧЕМУ LayerNorm ПЕРЕД подблоком (pre-norm): нормировать вход подблока
    надёжнее для глубоких сетей, чем классический post-norm.

    Форма не меняется: (B, T, d_model) -> (B, T, d_model).
    """

    def __init__(self, d_model: int, n_heads: int, d_ff: Optional[int] = None) -> None:
        super().__init__()
        if d_ff is None:
            d_ff = 4 * d_model
        self.ln1 = nn.LayerNorm(d_model)
        self.ln2 = nn.LayerNorm(d_model)
        self.attn = MultiHeadAttention(d_model, n_heads)
        self.mlp = nn.Sequential(
            nn.Linear(d_model, d_ff),
            nn.GELU(),
            nn.Linear(d_ff, d_model),
        )

    def forward(self, x: torch.Tensor) -> torch.Tensor:
        x = x + self.attn(self.ln1(x))
        x = x + self.mlp(self.ln2(x))
        return x
