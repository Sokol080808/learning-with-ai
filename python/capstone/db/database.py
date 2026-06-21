# ЭТАЛОННОЕ РЕШЕНИЕ (ветка reference). На ветке main здесь лежит стаб с NotImplementedError —
# его заполняет ученик. Этот файл существует только чтобы доказать, что задачи решаемы и что
# тесты (включая рандомизированные) зелёные на правильном коде. В main он НЕ попадает.
#
# Капстоун, слой 3 из 3 — ДВИЖОК (исполнитель запросов).
# Здесь живут ДАННЫЕ (таблицы в памяти) и СЕМАНТИКА (что значит «выполнить» каждый запрос).
# Слой опирается на готовые tokenize() и parse(): execute() — это конвейер
#     sql  --tokenize-->  токены  --parse-->  объект запроса  --исполнить-->  результат.
# Парсер уже отсеял синтаксический мусор; здесь ловим СЕМАНТИЧЕСКИЕ ошибки: таблица уже
# существует / её ещё нет / неверное число значений в INSERT / неизвестная колонка.
#
# МОДЕЛЬ ДАННЫХ (рекомендация, не догма): таблица — это (список колонок, список строк),
# где строка — dict col -> value. Все таблицы держи в одном dict: имя -> таблица.
# Значения храним типизированно: "30" -> int 30, "Alice" -> str "Alice" (см. coerce ниже).
#
# КОНТРАКТ execute по типу запроса:
#   CREATE -> None.  Завести пустую таблицу с указанными колонками.
#                    Повторный CREATE той же таблицы -> QueryError.
#   INSERT -> None.  Добавить строку. Неизвестная таблица -> QueryError.
#                    Число значений != числу колонок -> QueryError.
#   SELECT -> list[dict].  Список строк-словарей col->value. Применить, по порядку:
#                    WHERE-фильтр; проекцию колонок (SELECT name, age — только эти ключи,
#                    в указанном порядке; SELECT * — все колонки в порядке схемы);
#                    ORDER BY (ASC/DESC). Неизвестная таблица/колонка -> QueryError.
#   DELETE -> int.   Удалить строки, подходящие под WHERE (или ВСЕ, если WHERE нет).
#                    Вернуть число удалённых. Неизвестная таблица -> QueryError.
#
# СРАВНЕНИЯ в WHERE: = != < > <= >= над значениями строки. Сравнивай согласованно по типу
# (число с числом, слово со словом — coerce приводит обе стороны единообразно). Несколько
# условий через AND — это логическое И (строка проходит, ТОЛЬКО если выполнены ВСЕ).
#
# Что задействуешь из курса: ООП и инкапсуляция состояния (модуль 06); исключения
# (модуль 08); аннотации типов (модуль 13); из стандартной библиотеки — sorted/operator
# для ORDER BY и сравнений (модуль 11); генераторы удобны для фильтрации строк (модуль 09).

from __future__ import annotations

from typing import Any

import copy
import json
import operator
from contextlib import contextmanager
from pathlib import Path

from .tokenizer import tokenize
from .parser import parse, Create, Insert, Select, Delete, Condition


class QueryError(Exception):
    """Ошибка запроса: и синтаксическая (поднимает parser), и семантическая (поднимает движок).

    Это ЕДИНЫЙ тип ошибки, который видит пользователь мини-СУБД. REPL ловит именно его и
    печатает текст, не роняя программу. Реализовывать тут нечего — наследование от Exception
    уже даёт всё нужное; при желании можно оставить тело пустым (pass).
    """


def coerce(token: str) -> Any:
    """Привести строковый токен к значению: целое число -> int, иначе -> str (как есть).

    Это единая точка типизации значений во всей СУБД — и для INSERT, и для правых частей
    WHERE. Благодаря ей сравнение 30 > 25 идёт как числа, а "Bob" = "Bob" — как строки.

    Примеры:  coerce("30") == 30   (int);  coerce("Alice") == "Alice"   (str);
              coerce("-7") == -7   (int);  coerce("12abc") == "12abc"   (str, не число).
    Подсказка: «целое» здесь — то, что распознаёт int(token) (с учётом ведущего '-').
    """
    try:
        return int(token)
    except (ValueError, TypeError):
        return token


class Database:
    """Мини-СУБД: таблицы в оперативной памяти и исполнение SQL-подобных запросов.

    Состояние полностью внутри одного экземпляра — два разных Database() не делят данные
    (это важно для детерминизма тестов: каждый тест берёт свежую базу).
    """

    def __init__(self) -> None:
        """Создать ПУСТУЮ базу (никаких таблиц).

        Заведи внутреннее хранилище таблиц (например, self._tables: dict[str, ...]).
        Имя поля — на твой вкус, тесты лезут только через execute(), а не в потроха.
        """
        # Имя таблицы -> (список колонок, список строк). Строка — dict col -> типизированное значение.
        self._tables: dict[str, _Table] = {}

    def execute(self, sql: str) -> Any:
        """Выполнить один SQL-запрос и вернуть результат согласно контракту (см. шапку файла).

        Конвейер: tokenize(sql) -> parse(tokens) -> разобрать тип объекта запроса и
        выполнить соответствующую операцию (CREATE/INSERT/SELECT/DELETE).

        Возвращает:
          None        для CREATE и INSERT;
          list[dict]  для SELECT (строки как словари col->value, с учётом проекции/WHERE/ORDER BY);
          int         для DELETE (число удалённых строк).
        Бросает QueryError при любой синтаксической ИЛИ семантической ошибке.

        Примеры сессии (полный контракт — в test_database.py):
          db = Database()
          db.execute("CREATE TABLE users (id, name, age)")          # -> None
          db.execute("INSERT INTO users VALUES (1, Alice, 30)")     # -> None
          db.execute("INSERT INTO users VALUES (2, Bob, 20)")       # -> None
          db.execute("SELECT name FROM users WHERE age > 25")
              # -> [{"name": "Alice"}]
          db.execute("SELECT * FROM users ORDER BY age DESC")
              # -> [{"id":1,"name":"Alice","age":30}, {"id":2,"name":"Bob","age":20}]
          db.execute("DELETE FROM users WHERE id = 1")              # -> 1
        """
        query = parse(tokenize(sql))

        if isinstance(query, Create):
            return self._exec_create(query)
        if isinstance(query, Insert):
            return self._exec_insert(query)
        if isinstance(query, Select):
            return self._exec_select(query)
        if isinstance(query, Delete):
            return self._exec_delete(query)
        # Сюда не дойдём: parse возвращает только эти четыре типа.
        raise QueryError(f"неизвестный тип запроса: {type(query).__name__}")

    # --- семантические помощники ---------------------------------------------

    def _require_table(self, name: str) -> "_Table":
        table = self._tables.get(name)
        if table is None:
            raise QueryError(f"таблица '{name}' не существует")
        return table

    def _exec_create(self, q: Create) -> None:
        if q.table in self._tables:
            raise QueryError(f"таблица '{q.table}' уже существует")
        self._tables[q.table] = _Table(columns=list(q.columns), rows=[])
        return None

    def _exec_insert(self, q: Insert) -> None:
        table = self._require_table(q.table)
        if len(q.values) != len(table.columns):
            raise QueryError(
                f"число значений ({len(q.values)}) не совпадает с числом колонок "
                f"({len(table.columns)}) таблицы '{q.table}'"
            )
        row = {col: coerce(val) for col, val in zip(table.columns, q.values)}
        table.rows.append(row)
        return None

    def _exec_select(self, q: Select) -> list[dict]:
        table = self._require_table(q.table)

        # какие колонки оставить в результате (с проверкой существования)
        if q.columns == ["*"]:
            projection = list(table.columns)
        else:
            for col in q.columns:
                if col not in table.columns:
                    raise QueryError(f"неизвестная колонка '{col}' в таблице '{q.table}'")
            projection = list(q.columns)

        # WHERE-фильтр (ленивым генератором), затем материализуем
        selected = [row for row in table.rows if _row_matches(row, q.where, table)]

        # ORDER BY
        if q.order_by is not None:
            if q.order_by not in table.columns:
                raise QueryError(f"неизвестная колонка '{q.order_by}' в ORDER BY")
            selected = sorted(
                selected,
                key=operator.itemgetter(q.order_by),
                reverse=q.descending,
            )

        # проекция: оставляем только нужные ключи в указанном порядке
        return [{col: row[col] for col in projection} for row in selected]

    def _exec_delete(self, q: Delete) -> int:
        table = self._require_table(q.table)
        keep: list[dict] = []
        removed = 0
        for row in table.rows:
            if _row_matches(row, q.where, table):
                removed += 1
            else:
                keep.append(row)
        table.rows = keep
        return removed

    # --- майлстоун 6: ПЕРСИСТЕНТНОСТЬ (json + pathlib + with) ------------------
    # До сих пор база жила только в оперативной памяти: закрыл процесс — данные исчезли.
    # Эти два метода дают «выгрузку на диск» и «загрузку обратно». Это прямой повод свести
    # вместе модуль 12 (файлы и JSON) и модуль 08 (контекст-менеджеры: with гарантирует
    # закрытие файла даже при исключении). Формат на диске — обычный JSON, который json
    # сериализует «бесплатно»: наши значения — это int и str, ключи строки, строки — словари.
    #
    # ИНВАРИАНТ round-trip: Database.load(db.save(path)) == db. Именно его проверяет
    # property-тест: сохранили случайную базу, прочитали обратно — получили равную.

    def to_dict(self) -> dict[str, Any]:
        """Сериализуемый снимок всей базы: {имя_таблицы: {"columns": [...], "rows": [...]}}.

        Это «чистые данные» без объектов-обёрток — ровно то, что без потерь ложится в JSON
        и обратно. Используется и save(), и сравнением баз на равенство.
        """
        return {
            name: {"columns": list(t.columns), "rows": [dict(r) for r in t.rows]}
            for name, t in self._tables.items()
        }

    def save(self, path: str | Path) -> Path:
        """Записать базу в JSON-файл по пути path. Вернуть Path (удобно для round-trip).

        with гарантирует, что файл закроется даже если запись прервётся исключением.
        ensure_ascii=False — чтобы кириллица/юникод в значениях не превращались в \\uXXXX.
        """
        path = Path(path)
        with path.open("w", encoding="utf-8") as f:
            json.dump(self.to_dict(), f, ensure_ascii=False, indent=2)
        return path

    @classmethod
    def load(cls, path: str | Path) -> "Database":
        """Прочитать базу из JSON-файла и собрать НОВЫЙ экземпляр Database.

        classmethod, потому что метод КОНСТРУИРУЕТ объект (альтернативный конструктор),
        а не меняет существующий: естественно писать Database.load(path).
        """
        path = Path(path)
        with path.open("r", encoding="utf-8") as f:
            data = json.load(f)
        db = cls()
        for name, payload in data.items():
            db._tables[name] = _Table(
                columns=list(payload["columns"]),
                rows=[dict(r) for r in payload["rows"]],
            )
        return db

    def __eq__(self, other: object) -> bool:
        """Две базы равны, если совпадает их сериализуемое содержимое (таблицы и строки).

        Нужно прежде всего для round-trip-инварианта load(save(db)) == db в тестах.
        """
        if not isinstance(other, Database):
            return NotImplemented
        return self.to_dict() == other.to_dict()

    # --- майлстоун 7: КОНТЕКСТ-МЕНЕДЖЕР транзакции (атомарность с откатом) -----
    # Иногда пачку операций нужно применить «всё или ничего»: если посреди пачки вылетело
    # исключение, состояние должно вернуться к тому, что было ДО входа в блок. Это классика
    # транзакций. Контекст-менеджер (модуль 08) — идеальная форма: на входе делаем снимок,
    # на выходе либо фиксируем (нормальный выход), либо откатываемся (выход по исключению).
    #
    #     with db.transaction():
    #         db.execute("INSERT ...")
    #         db.execute("DELETE ...")   # если тут вылетит QueryError — обе операции откатятся
    #
    # @contextmanager превращает функцию-генератор в менеджер: код ДО yield — это __enter__,
    # код ПОСЛЕ — __exit__. try/except/raise внутри даёт нам откат и проброс ошибки наружу.

    @contextmanager
    def transaction(self):
        """Атомарная пачка операций: при исключении внутри блока изменения откатываются.

        Снимок делаем глубокой копией хранилища (copy.deepcopy), чтобы откат не зависел от
        того, мутировали ли мы строки на месте. При успешном выходе снимок просто
        отбрасывается — все изменения остаются. Исключение НЕ глотаем: пробрасываем дальше.
        """
        snapshot = copy.deepcopy(self._tables)
        try:
            yield self
        except BaseException:
            self._tables = snapshot  # откат к состоянию «до входа в блок»
            raise


# --- внутренние структуры данных и сравнение ---------------------------------

class _Table:
    """Таблица в памяти: упорядоченный список колонок и список строк-словарей."""

    __slots__ = ("columns", "rows")

    def __init__(self, columns: list[str], rows: list[dict]) -> None:
        self.columns = columns
        self.rows = rows


_COMPARATORS = {
    "=":  operator.eq,
    "!=": operator.ne,
    "<":  operator.lt,
    ">":  operator.gt,
    "<=": operator.le,
    ">=": operator.ge,
}


def _eval_condition(row: dict, cond: Condition, table: "_Table") -> bool:
    """Проверить одно условие WHERE против строки. Колонку проверяем на существование."""
    if cond.column not in table.columns:
        raise QueryError(f"неизвестная колонка '{cond.column}' в WHERE")
    left = row[cond.column]
    right = coerce(cond.value)
    compare = _COMPARATORS[cond.op]
    # Сравнение число-vs-строка для < > <= >= в Python кидает TypeError; для нашего контракта
    # такие данные не возникают (колонка типизирована единообразно). На = / != работает всегда.
    try:
        return compare(left, right)
    except TypeError:
        # Разнотипное сравнение порядка: считаем «не подходит» вместо падения.
        return False


def _row_matches(row: dict, conditions: list[Condition], table: "_Table") -> bool:
    """Логическое И всех условий (пустой список -> True: проходят все строки)."""
    return all(_eval_condition(row, cond, table) for cond in conditions)
