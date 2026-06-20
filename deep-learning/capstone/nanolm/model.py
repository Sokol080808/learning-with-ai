# ВНИМАНИЕ: здесь пишешь ТЫ. Реализуй так, чтобы приёмочные тесты капстоуна стали зелёными.
#
# nanoLM — крошечная GPT-подобная языковая модель уровня символов.
# Это вершина курса: ты собираешь полную авторегрессионную модель из кирпичиков,
# которые уже писал руками. Сердце — БЛОК ТРАНСФОРМЕРА из модуля 15 (причинное
# self-attention + MLP + LayerNorm + residual). Модули здесь не пакеты (нет
# __init__.py), импортировать оттуда нельзя — перенеси/перепиши блок сюда. Это
# полезно: соберёшь трансформер ещё раз, уже по памяти.
#
# Всё на CPU, во float32. Объяви слои в __init__, тела forward/generate — твоя работа.

from typing import Optional

import torch
import torch.nn as nn
import torch.nn.functional as F


class CausalSelfAttention(nn.Module):
    """Многоголовое ПРИЧИННОЕ self-attention (как в модуле 15).

    На каждой позиции t токен «смотрит» только на позиции <= t. Это достигается
    маской: запрещённые (будущие) позиции получают -inf в матрице внимания до
    softmax, и потому их вес = 0.
    """

    def __init__(self, d_model: int, n_heads: int, block_size: int) -> None:
        super().__init__()
        assert d_model % n_heads == 0, "d_model должно делиться на n_heads"
        self.n_heads = n_heads
        self.d_head = d_model // n_heads

        # Одна проекция сразу в Q, K, V (для всех голов).
        self.qkv = nn.Linear(d_model, 3 * d_model)
        self.proj = nn.Linear(d_model, d_model)

        # Нижнетреугольная причинная маска (1 — можно смотреть, 0 — нельзя).
        mask = torch.tril(torch.ones(block_size, block_size))
        self.register_buffer("causal_mask", mask.view(1, 1, block_size, block_size))

    def forward(self, x: torch.Tensor) -> torch.Tensor:
        B, T, C = x.shape
        q, k, v = self.qkv(x).split(C, dim=2)         # каждый (B, T, C)

        # (B, T, C) -> (B, n_heads, T, d_head)
        q = q.view(B, T, self.n_heads, self.d_head).transpose(1, 2)
        k = k.view(B, T, self.n_heads, self.d_head).transpose(1, 2)
        v = v.view(B, T, self.n_heads, self.d_head).transpose(1, 2)

        # Скалярные произведения, масштабирование на sqrt(d_head).
        att = (q @ k.transpose(-2, -1)) / (self.d_head ** 0.5)   # (B, nh, T, T)
        att = att.masked_fill(self.causal_mask[:, :, :T, :T] == 0, float("-inf"))
        att = F.softmax(att, dim=-1)

        out = att @ v                                  # (B, nh, T, d_head)
        out = out.transpose(1, 2).contiguous().view(B, T, C)
        return self.proj(out)


class Block(nn.Module):
    """Блок трансформера: pre-LN, причинное внимание и MLP с residual-связями."""

    def __init__(self, d_model: int, n_heads: int, block_size: int, d_ff: int) -> None:
        super().__init__()
        self.ln1 = nn.LayerNorm(d_model)
        self.attn = CausalSelfAttention(d_model, n_heads, block_size)
        self.ln2 = nn.LayerNorm(d_model)
        self.mlp = nn.Sequential(
            nn.Linear(d_model, d_ff),
            nn.GELU(),
            nn.Linear(d_ff, d_model),
        )

    def forward(self, x: torch.Tensor) -> torch.Tensor:
        x = x + self.attn(self.ln1(x))
        x = x + self.mlp(self.ln2(x))
        return x


class TransformerLM(nn.Module):
    """GPT-подобная языковая модель: эмбеддинги -> N блоков трансформера -> голова.

    Архитектура (см. README капстоуна — там же картинка):
        idx (B, T) целые индексы символов
          -> token embedding   (vocab_size -> d_model)   «что за символ»
           + positional embedding (block_size -> d_model) «на каком он месте»
          -> N блоков трансформера (causal self-attention + MLP, оба с LayerNorm и residual)
          -> финальный LayerNorm
          -> Linear(d_model -> vocab_size)  «голова»
          -> logits (B, T, vocab_size)

    __init__(vocab_size, block_size, d_model=64, n_heads=4, n_layers=2, d_ff=None):
      Создай и сохрани слои:
        - self.block_size = block_size  (понадобится в generate для обрезки контекста);
        - self.token_emb = nn.Embedding(vocab_size, d_model);
        - self.pos_emb   = nn.Embedding(block_size, d_model);
        - self.blocks    = nn.ModuleList([... N блоков ...])  — стопка из n_layers
          блоков трансформера (можно nn.Sequential, но ModuleList нагляднее);
        - self.ln_f      = nn.LayerNorm(d_model)  — финальная нормировка;
        - self.head      = nn.Linear(d_model, vocab_size)  — проекция в словарь.
      d_ff (скрытый размер MLP внутри блока) по умолчанию 4 * d_model.
      Требуй d_model % n_heads == 0 (головы должны сложиться обратно).

    Блок трансформера. Реализуй его как в модуле 15 — например отдельным
    nn.Module внутри этого файла:
        x = x + attn(LayerNorm1(x))   # ПРИЧИННОЕ self-attention + residual
        x = x + mlp(LayerNorm2(x))    # MLP (Linear->GELU->Linear) + residual
    Внимание ОБЯЗАНО быть причинным (causal): позиция t не видит позиции > t.
    Реализуй self-attention сам (как в модуле 15) или через готовые причинные
    кирпичики — главное, чтобы тест причинности прошёл.

    forward(idx) -> logits:
      Вход idx: LongTensor формы (B, T), T <= block_size.
      Шаги:
        1) tok = self.token_emb(idx)                     # (B, T, d_model)
        2) pos_ids = torch.arange(T, device=idx.device)  # (T,)
           pos = self.pos_emb(pos_ids)                    # (T, d_model), броадкастится
        3) x = tok + pos                                  # (B, T, d_model)
        4) прогони x через все блоки                      # (B, T, d_model)
        5) x = self.ln_f(x)
        6) logits = self.head(x)                          # (B, T, vocab_size)
      Верни logits. Лосс (кросс-энтропию) считаем СНАРУЖИ, в обучении:
        F.cross_entropy(logits.view(B*T, vocab_size), targets.view(B*T)).

    generate(idx, max_new_tokens) -> idx:
      Авторегрессионная генерация. Вход idx: (B, T0) — затравка. На каждом из
      max_new_tokens шагов:
        1) если контекст длиннее block_size, обрежь его справа:
           idx_cond = idx[:, -self.block_size:]
        2) logits = self(idx_cond)                       # (B, t, vocab_size)
        3) возьми логиты ПОСЛЕДНЕЙ позиции: logits[:, -1, :]  # (B, vocab_size)
        4) probs = softmax(этих логитов, dim=-1)
        5) next_id = torch.multinomial(probs, num_samples=1)  # (B, 1) — сэмплируем
        6) idx = torch.cat([idx, next_id], dim=1)        # дописали символ
      Верни итоговый idx формы (B, T0 + max_new_tokens).
      На инференсе генерацию оборачивают в torch.no_grad().

    Формы (B — батч, T — длина, d_model, V = vocab_size):
      idx: (B, T) -> logits: (B, T, V);  generate: (B, T0) -> (B, T0 + max_new_tokens).

    ПОЧЕМУ нужны positional embeddings: само внимание симметрично по позициям
    (переставь токены — веса просто переставятся), порядок оно не чувствует.
    Чтобы модель знала «кто за кем», к каждому токену добавляем обучаемый вектор
    его позиции. ПОЧЕМУ голова в vocab_size: на каждой позиции мы предсказываем
    следующий символ — значит нужен балл на КАЖДЫЙ символ алфавита.
    """

    def __init__(
        self,
        vocab_size: int,
        block_size: int,
        d_model: int = 64,
        n_heads: int = 4,
        n_layers: int = 2,
        d_ff: Optional[int] = None,
    ) -> None:
        super().__init__()
        assert d_model % n_heads == 0, "d_model должно делиться на n_heads"
        if d_ff is None:
            d_ff = 4 * d_model

        self.block_size = block_size
        self.token_emb = nn.Embedding(vocab_size, d_model)
        self.pos_emb = nn.Embedding(block_size, d_model)
        self.blocks = nn.ModuleList(
            [Block(d_model, n_heads, block_size, d_ff) for _ in range(n_layers)]
        )
        self.ln_f = nn.LayerNorm(d_model)
        self.head = nn.Linear(d_model, vocab_size)

    def forward(self, idx: torch.Tensor) -> torch.Tensor:
        B, T = idx.shape
        tok = self.token_emb(idx)                                  # (B, T, d_model)
        pos_ids = torch.arange(T, device=idx.device)               # (T,)
        pos = self.pos_emb(pos_ids)                                 # (T, d_model)
        x = tok + pos                                              # (B, T, d_model)
        for block in self.blocks:
            x = block(x)
        x = self.ln_f(x)
        logits = self.head(x)                                     # (B, T, vocab_size)
        return logits

    @torch.no_grad()
    def generate(self, idx: torch.Tensor, max_new_tokens: int) -> torch.Tensor:
        for _ in range(max_new_tokens):
            idx_cond = idx[:, -self.block_size:]                   # обрезаем контекст
            logits = self(idx_cond)                               # (B, t, vocab_size)
            logits = logits[:, -1, :]                             # (B, vocab_size)
            probs = F.softmax(logits, dim=-1)
            next_id = torch.multinomial(probs, num_samples=1)     # (B, 1)
            idx = torch.cat([idx, next_id], dim=1)
        return idx
