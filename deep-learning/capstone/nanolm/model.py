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
        raise NotImplementedError(
            "TODO: сохрани block_size; создай token_emb=nn.Embedding(vocab_size,d_model), "
            "pos_emb=nn.Embedding(block_size,d_model), стопку из n_layers блоков "
            "трансформера (как в модуле 15, причинное внимание), ln_f=nn.LayerNorm(d_model), "
            "head=nn.Linear(d_model,vocab_size). d_ff по умолчанию 4*d_model; "
            "проверь d_model % n_heads == 0"
        )

    def forward(self, idx: torch.Tensor) -> torch.Tensor:
        raise NotImplementedError(
            "TODO: tok=token_emb(idx); pos=pos_emb(arange(T)); x=tok+pos; "
            "прогони через блоки; x=ln_f(x); верни head(x) формы (B,T,vocab_size)"
        )

    @torch.no_grad()
    def generate(self, idx: torch.Tensor, max_new_tokens: int) -> torch.Tensor:
        raise NotImplementedError(
            "TODO: max_new_tokens раз: обрежь контекст до block_size; logits=self(idx); "
            "возьми последнюю позицию; softmax; multinomial -> next_id; cat к idx. "
            "Верни idx формы (B, T0 + max_new_tokens)"
        )
