# Пакет tasks — ядро капстоуна.
# Реэкспорт: позволяет писать `from tasks import TaskStore, execute`
# вместо `from tasks.store import TaskStore` / `from tasks.commands import execute`.
# Так app.py и любой внешний код зависят от пакета, а не от внутренней раскладки файлов.

from tasks.store import TaskStore
from tasks.commands import execute

__all__ = ["TaskStore", "execute"]
