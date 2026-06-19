# Эти тесты трогать не нужно — это эталон поведения (приёмочные тесты капстоуна).
#
# Майлстоун 3 — обучение. Проверяем, что связка токенизатор + данные + модель
# реально УЧИТСЯ: на крошечном seeded корпусе несколько шагов заметно снижают
# лосс. Точное число не требуем — требуем итоговый < 0.5 * начального.
# Всё детерминировано, крошечно, на CPU.

import torch
import torch.nn.functional as F

from nanolm.tokenizer import CharTokenizer
from nanolm.model import TransformerLM
from nanolm.data import get_batch


# Крошечный корпус с явной структурой — модель легко ловит статистику символов.
CORPUS = "abcabcabcabcabcabcabcabcabcabc abcabc abcabcabc"


def _cross_entropy_loss(model, x, y):
    logits = model(x)                       # (B, T, V)
    B, T, V = logits.shape
    return F.cross_entropy(logits.view(B * T, V), y.view(B * T))


def test_training_reduces_loss():
    torch.manual_seed(0)

    tok = CharTokenizer().build(CORPUS)
    data = torch.tensor(tok.encode(CORPUS), dtype=torch.long)

    block_size, batch_size = 8, 16
    model = TransformerLM(
        vocab_size=tok.vocab_size,
        block_size=block_size,
        d_model=32,
        n_heads=4,
        n_layers=2,
    )
    opt = torch.optim.AdamW(model.parameters(), lr=3e-3)

    # Фиксированный «оценочный» батч, на котором меряем лосс до и после.
    xe, ye = get_batch(data, block_size, batch_size, seed=999)

    model.eval()
    with torch.no_grad():
        initial_loss = _cross_entropy_loss(model, xe, ye).item()

    model.train()
    for step in range(200):
        x, y = get_batch(data, block_size, batch_size, seed=step)
        loss = _cross_entropy_loss(model, x, y)
        opt.zero_grad()
        loss.backward()
        opt.step()

    model.eval()
    with torch.no_grad():
        final_loss = _cross_entropy_loss(model, xe, ye).item()

    # Обучение должно ЗАМЕТНО снижать лосс.
    assert final_loss < 0.5 * initial_loss


def test_initial_loss_is_reasonable():
    # До обучения лосс на случайной модели ~ ln(vocab_size) (равномерное угадывание).
    # Проверяем порядок величины: он не должен быть абсурдно большим/маленьким.
    import math

    torch.manual_seed(0)
    tok = CharTokenizer().build(CORPUS)
    data = torch.tensor(tok.encode(CORPUS), dtype=torch.long)

    block_size, batch_size = 8, 16
    model = TransformerLM(
        vocab_size=tok.vocab_size,
        block_size=block_size,
        d_model=32,
        n_heads=4,
        n_layers=2,
    )
    x, y = get_batch(data, block_size, batch_size, seed=0)
    model.eval()
    with torch.no_grad():
        loss = _cross_entropy_loss(model, x, y).item()

    uniform = math.log(tok.vocab_size)
    # достаточно мягкие границы — проверяем порядок, а не точное число
    assert 0.3 * uniform < loss < 3.0 * uniform


def test_gradients_flow_to_all_params():
    # после одного backward у всех обучаемых параметров есть градиент —
    # значит граф связен и ничего не «оторвано» от лосса.
    torch.manual_seed(0)
    tok = CharTokenizer().build(CORPUS)
    data = torch.tensor(tok.encode(CORPUS), dtype=torch.long)

    model = TransformerLM(
        vocab_size=tok.vocab_size,
        block_size=8,
        d_model=32,
        n_heads=4,
        n_layers=2,
    )
    x, y = get_batch(data, 8, 8, seed=0)
    loss = _cross_entropy_loss(model, x, y)
    loss.backward()

    for name, p in model.named_parameters():
        assert p.grad is not None, f"нет градиента у параметра {name}"
        assert torch.isfinite(p.grad).all(), f"нефинитный градиент у {name}"
