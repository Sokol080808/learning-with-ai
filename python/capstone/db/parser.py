# ЭТАЛОННОЕ РЕШЕНИЕ (ветка reference). На ветке main здесь лежит стаб с NotImplementedError —
# его заполняет ученик. Этот файл существует только чтобы доказать, что задачи решаемы и что
# тесты (включая рандомизированные) зелёные на правильном коде. В main он НЕ попадает.
#
# Капстоун, слой 2 из 3 — ПАРСЕР.
# Задача слоя: из ПЛОСКОГО списка токенов собрать СТРУКТУРУ — объект запроса, в котором
# уже разложено по полочкам, ЧТО хочет пользователь. Парсер не трогает данные таблиц —
# он лишь понимает грамматику. Если грамматика нарушена (мусор, нет FROM, незакрытая
# скобка, оборванное условие) — парсер обязан поднять QueryError. Это твой «контроль на
# входе»: дальше, в database.py, можно считать, что объект запроса корректен по форме.
#
# ФОРМА РЕЗУЛЬТАТА. Мы фиксируем её через dataclass'ы ниже (модуль 07 курса —
# «Dataclasses и протоколы»). Парсер возвращает ОДИН из четырёх объектов: Create, Insert,
# Select, Delete. Тесты сравнивают результат через ==, а dataclass даёт __eq__ бесплатно —
# поэтому менять эти классы не нужно, нужно лишь правильно их СОБИРАТЬ из токенов.
#
# КЛЮЧЕВЫЕ СЛОВА приводим к ВЕРХНЕМУ регистру при сравнении (SELECT == select == Select),
# но имена таблиц/колонок и значения сохраняем КАК ЕСТЬ (users, Alice — без изменений).
#
# Грамматика (то, что нужно распознать):
#   CREATE TABLE <table> ( <col> , <col> , ... )
#   INSERT INTO <table> VALUES ( <val> , <val> , ... )
#   SELECT <* | col , col ...> FROM <table> [ WHERE <условие> ] [ ORDER BY <col> [ASC|DESC] ]
#   DELETE FROM <table> [ WHERE <условие> ]
#
# УСЛОВИЕ (WHERE) — это одна или несколько ЭЛЕМЕНТАРНЫХ проверок, склеенных через AND:
#   <col> <op> <value>  [ AND <col> <op> <value> ... ]
#   op ∈ { =, !=, <, >, <=, >= }.  Мы храним каждую проверку как Condition.
#
# ЗНАЧЕНИЯ (value) в INSERT и в WHERE: парсер на этом слое МОЖЕТ оставить их строками —
# превращение "30" -> int делается единообразно в database.py (см. coerce там). Но если
# тебе удобнее приводить тип уже здесь — это не запрещено; смотри, что именно сравнивают
# тесты test_parser.py, и держи ровно тот контракт.
#
# КОГДА ПОДНИМАТЬ QueryError (синтаксис!): пустой ввод; неизвестное первое слово; нет
# обязательного ключевого слова (TABLE/INTO/VALUES/FROM/BY); несбалансированные или
# отсутствующие скобки; пустой список колонок/значений; оборванное условие; лишние токены
# в конце. Сообщение — человекочитаемое (его увидит ученик в REPL).
#
# Подсказка по структуре кода (один из способов): сделай маленький курсор по списку
# токенов (индекс + вспомогательные expect()/peek()), и отдельную функцию parse_conditions(),
# которую переиспользуют и SELECT, и DELETE. Это прямой повод для ООП/функций (модули 05–06).

from __future__ import annotations

from dataclasses import dataclass, field

# QueryError «живёт» в database.py (там же, где Database) и реэкспортируется пакетом.
# Чтобы не словить циклический импорт (database импортирует parse отсюда), импортируй
# QueryError ВНУТРИ функции parse() — например так:
#     from .database import QueryError
# Это валидный приём: к моменту вызова parse() оба модуля уже загружены.


# --- AST: объекты запроса --------------------------------------------------
# Эти классы — КОНТРАКТ формы. Тесты создают точно такие же объекты и сравнивают ==.
# Поля и их порядок менять нельзя (иначе разъедутся тесты). Реализовывать тут нечего —
# вся работа в функции parse().

@dataclass(frozen=True)
class Condition:
    """Одна элементарная проверка WHERE: <column> <op> <value>.

    op — строка-оператор из набора {"=", "!=", "<", ">", "<=", ">="}.
    value — пока строка-токен (например "30" или "Bob"); приведение к int делает database.py.
    """
    column: str
    op: str
    value: str


@dataclass(frozen=True)
class Create:
    """CREATE TABLE <table> (<columns...>)."""
    table: str
    columns: list[str]


@dataclass(frozen=True)
class Insert:
    """INSERT INTO <table> VALUES (<values...>). values — список строк-токенов."""
    table: str
    values: list[str]


@dataclass(frozen=True)
class Select:
    """SELECT <columns | *> FROM <table> [WHERE ...] [ORDER BY <col> <ASC|DESC>].

    columns: ["*"] для SELECT * , иначе список имён колонок.
    where:   список Condition (пустой, если WHERE нет); все условия склеены логическим AND.
    order_by: имя колонки или None, если ORDER BY нет.
    descending: True для DESC, False для ASC/по умолчанию.
    """
    table: str
    columns: list[str]
    where: list[Condition] = field(default_factory=list)
    order_by: str | None = None
    descending: bool = False


@dataclass(frozen=True)
class Delete:
    """DELETE FROM <table> [WHERE ...]. where — список Condition (пустой, если WHERE нет)."""
    table: str
    where: list[Condition] = field(default_factory=list)


# Тип «любой объект запроса» — пригодится для аннотаций в database.py.
Query = Create | Insert | Select | Delete


def parse(tokens: list[str]) -> Query:
    """Разобрать список токенов в один объект запроса (Create/Insert/Select/Delete).

    На вход — то, что вернул tokenize(). На выход — заполненный dataclass из этого модуля.
    Любая СИНТАКСИЧЕСКАЯ ошибка (см. список в шапке файла) -> raise QueryError.

    Примеры (полный контракт — в test_parser.py):
      parse(["CREATE","TABLE","users","(","id",",","name",")"])
          == Create(table="users", columns=["id", "name"])

      parse(["SELECT","*","FROM","users"])
          == Select(table="users", columns=["*"])

      parse(["SELECT","name",",","age","FROM","users","WHERE","age",">","25"])
          == Select(table="users", columns=["name","age"],
                    where=[Condition("age", ">", "25")])

      parse(["SELECT","*","FROM","users","ORDER","BY","age","DESC"])
          == Select(table="users", columns=["*"], order_by="age", descending=True)

      parse(["DELETE","FROM","users","WHERE","id","=","1"])
          == Delete(table="users", where=[Condition("id", "=", "1")])

    Возвращает: Query (один из четырёх dataclass'ов).
    Бросает:   QueryError при любой синтаксической ошибке.
    """
    # Импорт ВНУТРИ функции — чтобы разорвать циклический импорт (database импортирует parse).
    from .database import QueryError

    cur = _Cursor(tokens, QueryError)

    if cur.at_end():
        raise QueryError("пустой запрос")

    head = cur.peek().upper()
    if head == "CREATE":
        query = _parse_create(cur)
    elif head == "INSERT":
        query = _parse_insert(cur)
    elif head == "SELECT":
        query = _parse_select(cur)
    elif head == "DELETE":
        query = _parse_delete(cur)
    else:
        raise QueryError(f"неизвестная команда: {cur.peek()!r}")

    if not cur.at_end():
        raise QueryError(f"лишние токены в конце запроса: {tokens[cur.pos:]!r}")
    return query


# --- внутренняя кухня парсера ------------------------------------------------

class _Cursor:
    """Маленький курсор по списку токенов: peek/next/expect с понятными ошибками."""

    def __init__(self, tokens: list[str], error_cls) -> None:
        self.tokens = tokens
        self.pos = 0
        self._error = error_cls

    def at_end(self) -> bool:
        return self.pos >= len(self.tokens)

    def peek(self) -> str:
        if self.at_end():
            raise self._error("неожиданный конец запроса")
        return self.tokens[self.pos]

    def peek_upper(self) -> str | None:
        if self.at_end():
            return None
        return self.tokens[self.pos].upper()

    def next(self) -> str:
        tok = self.peek()
        self.pos += 1
        return tok

    def expect_keyword(self, keyword: str) -> None:
        tok = self.next()
        if tok.upper() != keyword:
            raise self._error(f"ожидалось ключевое слово {keyword!r}, получено {tok!r}")

    def expect_symbol(self, symbol: str) -> None:
        tok = self.next()
        if tok != symbol:
            raise self._error(f"ожидался {symbol!r}, получено {tok!r}")

    def next_name(self) -> str:
        """Имя таблицы/колонки/значение: любой токен, не являющийся пунктуатором."""
        tok = self.next()
        if tok in ("(", ")", ",") or tok in _OPERATORS:
            raise self._error(f"ожидалось имя, получен символ {tok!r}")
        return tok


_OPERATORS = {"=", "!=", "<", ">", "<=", ">="}


def _parse_name_list(cur: _Cursor) -> list[str]:
    """Разобрать ( name , name , ... ) — минимум одно имя, скобки обязательны."""
    cur.expect_symbol("(")
    names: list[str] = []
    while True:
        names.append(cur.next_name())
        sep = cur.next()
        if sep == ")":
            break
        if sep != ",":
            raise cur._error(f"ожидалась ',' или ')', получено {sep!r}")
    if not names:
        raise cur._error("пустой список в скобках")
    return names


def _parse_create(cur: _Cursor) -> Create:
    cur.expect_keyword("CREATE")
    cur.expect_keyword("TABLE")
    table = cur.next_name()
    columns = _parse_name_list(cur)
    return Create(table=table, columns=columns)


def _parse_insert(cur: _Cursor) -> Insert:
    cur.expect_keyword("INSERT")
    cur.expect_keyword("INTO")
    table = cur.next_name()
    cur.expect_keyword("VALUES")
    values = _parse_name_list(cur)
    return Insert(table=table, values=values)


def _parse_conditions(cur: _Cursor) -> list[Condition]:
    """Разобрать одно или несколько условий, склеенных через AND."""
    conditions: list[Condition] = []
    while True:
        column = cur.next_name()
        op = cur.next()
        if op not in _OPERATORS:
            raise cur._error(f"ожидался оператор сравнения, получено {op!r}")
        value = cur.next_name()
        conditions.append(Condition(column, op, value))
        if cur.peek_upper() == "AND":
            cur.next()  # съесть AND и продолжить
            continue
        break
    return conditions


def _parse_where(cur: _Cursor) -> list[Condition]:
    if cur.peek_upper() == "WHERE":
        cur.next()
        return _parse_conditions(cur)
    return []


def _parse_select(cur: _Cursor) -> Select:
    cur.expect_keyword("SELECT")

    columns: list[str] = []
    if cur.peek() == "*":
        cur.next()
        columns = ["*"]
    else:
        while True:
            columns.append(cur.next_name())
            if cur.peek_upper() == "FROM":
                break
            cur.expect_symbol(",")
    if not columns:
        raise cur._error("пустой список колонок в SELECT")

    cur.expect_keyword("FROM")
    table = cur.next_name()

    where = _parse_where(cur)

    order_by: str | None = None
    descending = False
    if cur.peek_upper() == "ORDER":
        cur.next()
        cur.expect_keyword("BY")
        order_by = cur.next_name()
        nxt = cur.peek_upper()
        if nxt == "DESC":
            cur.next()
            descending = True
        elif nxt == "ASC":
            cur.next()
            descending = False

    return Select(table=table, columns=columns, where=where,
                  order_by=order_by, descending=descending)


def _parse_delete(cur: _Cursor) -> Delete:
    cur.expect_keyword("DELETE")
    cur.expect_keyword("FROM")
    table = cur.next_name()
    where = _parse_where(cur)
    return Delete(table=table, where=where)
