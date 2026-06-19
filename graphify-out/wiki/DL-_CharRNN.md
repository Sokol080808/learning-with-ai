# DL: CharRNN

> 26 nodes

## Key Concepts

- **CharRNN** (15 connections) — `deep-learning/modules/14-sequence-models/seqmodels.py`
- **test_seqmodels.py** (12 connections) — `deep-learning/modules/14-sequence-models/test_seqmodels.py`
- **train_step()** (5 connections) — `deep-learning/modules/14-sequence-models/seqmodels.py`
- **_toy_batch()** (5 connections) — `deep-learning/modules/14-sequence-models/test_seqmodels.py`
- **.forward()** (3 connections) — `deep-learning/modules/14-sequence-models/seqmodels.py`
- **test_train_step_returns_float()** (3 connections) — `deep-learning/modules/14-sequence-models/test_seqmodels.py`
- **test_train_step_updates_parameters()** (3 connections) — `deep-learning/modules/14-sequence-models/test_seqmodels.py`
- **test_train_step_matches_manual_loss_value()** (3 connections) — `deep-learning/modules/14-sequence-models/test_seqmodels.py`
- **seqmodels.py** (2 connections) — `deep-learning/modules/14-sequence-models/seqmodels.py`
- **.__init__()** (2 connections) — `deep-learning/modules/14-sequence-models/seqmodels.py`
- **Tensor** (2 connections) — `deep-learning/modules/14-sequence-models/seqmodels.py`
- **test_charrnn_is_nn_module()** (2 connections) — `deep-learning/modules/14-sequence-models/test_seqmodels.py`
- **test_charrnn_has_expected_layers()** (2 connections) — `deep-learning/modules/14-sequence-models/test_seqmodels.py`
- **test_charrnn_output_shape()** (2 connections) — `deep-learning/modules/14-sequence-models/test_seqmodels.py`
- **test_charrnn_output_shape_other_sizes()** (2 connections) — `deep-learning/modules/14-sequence-models/test_seqmodels.py`
- **test_charrnn_output_is_float_tensor()** (2 connections) — `deep-learning/modules/14-sequence-models/test_seqmodels.py`
- **test_charrnn_batch_dim_independent()** (2 connections) — `deep-learning/modules/14-sequence-models/test_seqmodels.py`
- **test_training_reduces_loss_on_memorized_sequence()** (2 connections) — `deep-learning/modules/14-sequence-models/test_seqmodels.py`
- **test_training_reduces_loss_on_small_batch()** (2 connections) — `deep-learning/modules/14-sequence-models/test_seqmodels.py`
- **Module** (1 connections) — `deep-learning/modules/14-sequence-models/seqmodels.py`
- **Optimizer** (1 connections) — `deep-learning/modules/14-sequence-models/seqmodels.py`
- **Символьная рекуррентная сеть: Embedding -> RNN -> Linear.      Архитектура (три** (1 connections) — `deep-learning/modules/14-sequence-models/seqmodels.py`
- **Создай три слоя (embed, rnn, fc) и сохрани размеры.          Не забудь super()._** (1 connections) — `deep-learning/modules/14-sequence-models/seqmodels.py`
- **Прямой проход: индексы (B, T) -> логиты (B, T, vocab_size).          Шаги: embed** (1 connections) — `deep-learning/modules/14-sequence-models/seqmodels.py`
- **Один шаг обучения. Возвращает значение loss НА ЭТОМ батче ДО шага (как float).** (1 connections) — `deep-learning/modules/14-sequence-models/seqmodels.py`
- *... and 1 more nodes in this community*

## Relationships

- No strong cross-community connections detected

## Source Files

- `deep-learning/modules/14-sequence-models/seqmodels.py`
- `deep-learning/modules/14-sequence-models/test_seqmodels.py`

## Audit Trail

- EXTRACTED: 56 (72%)
- INFERRED: 22 (28%)
- AMBIGUOUS: 0 (0%)

---

*Part of the graphify knowledge wiki. See [[index]] to navigate.*