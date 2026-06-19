# Пакет db — мини-СУБД.
#
# Этот файл — публичное «лицо» пакета: что снаружи пишут `from db import Database, QueryError`,
# а не лазают по внутренним модулям. Менять его обычно НЕ нужно — он уже собран правильно.
# (Модуль 10 курса — «Модули и пакеты»: __init__.py и реэкспорт имён.)

from .database import Database, QueryError

__all__ = ["Database", "QueryError"]
