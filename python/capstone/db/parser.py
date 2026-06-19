# ВНИМАНИЕ: здесь пишешь ТЫ. Это СТАБ — реализуй parse так, чтобы тесты стали зелёными.
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
    raise NotImplementedError(
        "TODO: разбери токены в Create/Insert/Select/Delete; синтаксические ошибки -> raise QueryError. "
        "Заведи курсор по токенам и общую parse_conditions() для WHERE. См. test_parser.py"
    )
