# ВНИМАНИЕ: здесь пишешь ТЫ. Это СТАБ — реализуй Database.execute и помощники так, чтобы
# тесты стали зелёными.
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
from pathlib import Path

from .tokenizer import tokenize
from .parser import parse


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
    raise NotImplementedError(
        "TODO: верни int(token), если токен — целое число (в т.ч. отрицательное), иначе сам token (str). "
        "См. тесты на coerce/типы значений в test_database.py"
    )


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
        raise NotImplementedError(
            "TODO: инициализируй пустое хранилище таблиц (например self._tables = {}). См. test_database.py"
        )

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
        raise NotImplementedError(
            "TODO: проведи sql через tokenize -> parse и выполни CREATE/INSERT/SELECT/DELETE; "
            "семантические ошибки -> raise QueryError. Разнеси ветки по приватным методам. См. test_database.py"
        )

    # --- майлстоун 6: ПЕРСИСТЕНТНОСТЬ (json + pathlib + with) ------------------
    # До сих пор база жила только в оперативной памяти: закрыл процесс — данные исчезли.
    # Эти два метода дают «выгрузку на диск» и «загрузку обратно». Это прямой повод свести
    # вместе модуль 12 (файлы и JSON) и модуль 08 (контекст-менеджеры: with гарантирует
    # закрытие файла даже при исключении). Формат на диске — обычный JSON:
    # {имя_таблицы: {"columns": [...], "rows": [{col: value, ...}, ...]}, ...}
    #
    # ИНВАРИАНТ round-trip: Database.load(db.save(path)) == db.
    # Для == нужен __eq__, для __eq__ удобен вспомогательный to_dict().

    def to_dict(self) -> dict[str, Any]:
        """Сериализуемый снимок всей базы: {имя_таблицы: {"columns": [...], "rows": [...]}}.

        Формат: каждая таблица — словарь с ключами "columns" (список строк) и "rows"
        (список словарей col->value). Именно этот формат json.dump/load использует без потерь,
        а __eq__ применяет для сравнения двух баз.

        Подсказка: пройди по self._tables и для каждой таблицы собери нужный словарь.
        Не забудь скопировать списки (list(...)), иначе внешние изменения мутируют внутренности.
        """
        raise NotImplementedError(
            "TODO: верни dict вида {name: {'columns': [...], 'rows': [...]}} для каждой таблицы. "
            "Значения строк — уже типизированы (int/str), копируй как есть. См. test_persistence_props.py"
        )

    def save(self, path: str | Path) -> Path:
        """Записать базу в JSON-файл по пути path. Вернуть Path (удобно для round-trip).

        Шаги:
        1. Path(path) — нормализуй аргумент в Path.
        2. with path.open("w", encoding="utf-8") as f: — открой файл для записи.
        3. json.dump(self.to_dict(), f, ensure_ascii=False, indent=2) — запиши снимок.
        4. Верни path.

        Подсказка: не забудь import json (добавь в импорты вверху файла).
        """
        raise NotImplementedError(
            "TODO: сериализуй self.to_dict() в JSON-файл по пути path, верни Path(path). "
            "Используй with ... open(...) и json.dump. См. test_persistence_props.py"
        )

    @classmethod
    def load(cls, path: str | Path) -> "Database":
        """Прочитать базу из JSON-файла и вернуть НОВЫЙ экземпляр Database.

        Это classmethod (альтернативный конструктор): Database.load(path).
        Шаги:
        1. Path(path) — нормализуй.
        2. with path.open("r", encoding="utf-8") as f: data = json.load(f).
        3. db = cls() — новый пустой экземпляр.
        4. Воссоздай self._tables из data: для каждого имени/payload заполни таблицу.
        5. Верни db.

        Подсказка: структура data зеркалит to_dict() — те же ключи "columns" и "rows".
        """
        raise NotImplementedError(
            "TODO: прочитай JSON из path, создай cls(), заполни _tables из данных, верни db. "
            "Формат файла — тот же, что to_dict() записывает. См. test_persistence_props.py"
        )

    def __eq__(self, other: object) -> bool:
        """Две базы равны, если совпадает их сериализуемое содержимое (to_dict() одинаков).

        Нужно для инварианта load(save(db)) == db в тестах.
        Подсказка: проверь isinstance(other, Database), затем сравни self.to_dict() == other.to_dict().
        """
        raise NotImplementedError(
            "TODO: верни True если isinstance(other, Database) и self.to_dict() == other.to_dict(), "
            "иначе NotImplemented. См. test_persistence_props.py"
        )

    # --- майлстоун 7: КОНТЕКСТ-МЕНЕДЖЕР транзакции (атомарность с откатом) -----
    # Пачка операций «всё или ничего»: если посреди блока вылетело исключение,
    # база возвращается к состоянию ДО входа в блок.
    #
    #     with db.transaction():
    #         db.execute("INSERT ...")
    #         db.execute("DELETE ...")   # если тут QueryError — обе операции откатятся
    #
    # @contextmanager превращает генератор в менеджер: код до yield — __enter__,
    # код после — __exit__. try/except/raise даёт откат + проброс исключения наружу.

    def transaction(self):
        """Атомарная пачка операций: при любом исключении внутри блока изменения откатываются.

        Реализация-скелет:
          1. Сделай глубокую копию self._tables (copy.deepcopy) — это снимок состояния.
          2. try: yield self  — передай управление телу блока with.
          3. except BaseException: восстанови self._tables = snapshot; raise  — откат + проброс.
          4. При нормальном выходе (нет исключения) снимок просто отбрасывается.

        Превратить в контекст-менеджер можно двумя способами:
          а) @contextmanager из contextlib (декоратор над генератором — см. модуль 08);
          б) класс с __enter__/__exit__.

        Подсказка: не забудь import copy и from contextlib import contextmanager.
        """
        raise NotImplementedError(
            "TODO: реализуй контекст-менеджер транзакции: сохрани снимок _tables, "
            "yield self, при исключении восстанови снимок и reraise. См. test_persistence_props.py"
        )
