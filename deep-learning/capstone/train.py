# Готовый рабочий скрипт обучения nanoLM. ПИСАТЬ ЕГО НЕ НУЖНО.
#
# Он опирается на ТВОИ реализации: CharTokenizer (tokenizer.py), TransformerLM
# (model.py) и get_batch (data.py). Пока они не реализованы — скрипт упадёт с
# NotImplementedError. Как только майлстоуны 1–3 зелёные, запусти:
#
#     ./deep-learning/run.sh capstone      # сначала прогнать тесты
#     python deep-learning/capstone/train.py   # затем своими глазами увидеть обучение и генерацию
#
# Всё на CPU, корпус крошечный, сиды зафиксированы. Скрипт обучает модель на
# встроенной строке-корпусе и в конце генерирует продолжение по затравке.

import os
import sys

import torch
import torch.nn.functional as F

# Чтобы `from nanolm ...` работал и при прямом запуске скрипта.
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))

from nanolm.tokenizer import CharTokenizer
from nanolm.model import TransformerLM
from nanolm.data import get_batch


# --- Крошечный встроенный корпус. Можешь заменить на свой текст. -------------
CORPUS = (
    "to be or not to be that is the question\n"
    "whether tis nobler in the mind to suffer\n"
    "the slings and arrows of outrageous fortune\n"
    "or to take arms against a sea of troubles\n"
)

# --- Гиперпараметры (маленькие — это CPU и игрушечный масштаб). --------------
BLOCK_SIZE = 16     # длина контекста (сколько символов модель видит за раз)
BATCH_SIZE = 16
D_MODEL = 64
N_HEADS = 4
N_LAYERS = 2
STEPS = 300
LR = 3e-3
SEED = 0


def main() -> None:
    torch.manual_seed(SEED)

    # 1) Токенизатор и данные.
    tok = CharTokenizer().build(CORPUS)
    print(f"Размер словаря (уникальных символов): {tok.vocab_size}")

    data = torch.tensor(tok.encode(CORPUS), dtype=torch.long)
    print(f"Длина корпуса в символах: {len(data)}")

    # 2) Модель.
    model = TransformerLM(
        vocab_size=tok.vocab_size,
        block_size=BLOCK_SIZE,
        d_model=D_MODEL,
        n_heads=N_HEADS,
        n_layers=N_LAYERS,
    )
    n_params = sum(p.numel() for p in model.parameters())
    print(f"Параметров в модели: {n_params}")

    # 3) Цикл обучения (модули 12 и 16): батч -> forward -> CE-лосс -> backward -> шаг.
    opt = torch.optim.AdamW(model.parameters(), lr=LR)

    model.train()
    for step in range(1, STEPS + 1):
        # Сид зависит от шага -> каждый шаг новый батч, но всё воспроизводимо.
        x, y = get_batch(data, BLOCK_SIZE, BATCH_SIZE, seed=SEED + step)
        logits = model(x)                       # (B, T, vocab_size)
        B, T, V = logits.shape
        loss = F.cross_entropy(logits.view(B * T, V), y.view(B * T))

        opt.zero_grad()
        loss.backward()
        opt.step()

        if step == 1 or step % 50 == 0:
            print(f"шаг {step:4d} | лосс {loss.item():.4f}")

    # 4) Генерация (майлстоун 4): дописываем текст по затравке.
    model.eval()
    start = "to be"
    idx = torch.tensor([tok.encode(start)], dtype=torch.long)   # (1, len(start))
    out = model.generate(idx, max_new_tokens=120)               # (1, len(start)+120)
    text = tok.decode(out[0].tolist())

    print("\n--- Сгенерированный текст ---")
    print(text)
    print("----------------------------")
    print(
        "\nНа таком крошечном корпусе модель просто запоминает его — это нормально\n"
        "и наглядно. Увеличь корпус/STEPS/слои и посмотри, как меняются лосс и текст."
    )


if __name__ == "__main__":
    main()
