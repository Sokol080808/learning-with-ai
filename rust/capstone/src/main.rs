// Это ГОТОВЫЙ рабочий REPL (Read-Eval-Print Loop). Трогать его не нужно — он уже
// «крутит» цикл ввод→ответ вокруг твоей функции `execute` из библиотеки. Пока `execute`
// и методы KvStore возвращают todo!(), запуск будет паниковать при первой же команде;
// как только ты их реализуешь — REPL оживёт.
//
// Запуск из корня воркспейса:  cargo run -p kvstore
// (или собрать и запустить бинарь напрямую — см. README.md).
//
// Архитектурная мысль: ВЕСЬ смысл команд живёт в библиотеке (execute + KvStore). main.rs
// занят только вводом/выводом: прочитать строку, отдать её execute, напечатать ответ.
// Такое разделение («тонкий бинарь над толстой библиотекой») — почему мы можем тестировать
// логику приёмочными тестами, вообще не запуская сам REPL.

use std::io::{self, BufRead, Write};

use kvstore::{execute, KvStore};

fn main() {
    let mut store = KvStore::new();

    // Захватываем stdin/stdout один раз и блокируем — так быстрее, чем брать их на каждой
    // строке. `lock()` даёт нам монопольный доступ к потоку на всё время цикла.
    let stdin = io::stdin();
    let mut stdout = io::stdout();

    print_banner(&mut stdout);

    // Идём по вводу построчно. `lines()` отдаёт `Result<String, io::Error>`: на каждой
    // итерации строка уже без завершающего '\n'. Ctrl-D (конец ввода) завершит цикл сам.
    for line in stdin.lock().lines() {
        let line = match line {
            Ok(l) => l,
            Err(e) => {
                // Ошибка чтения ввода — редкость, но обработаем честно, а не паникой.
                eprintln!("ошибка чтения ввода: {e}");
                break;
            }
        };

        let trimmed = line.trim();

        // Пустую строку просто пропускаем — не дёргаем execute, не печатаем ничего.
        if trimmed.is_empty() {
            continue;
        }

        // QUIT — забота REPL, а не языка команд: выходим из цикла, не зовём execute.
        // Сравниваем без учёта регистра, чтобы и `quit`, и `QUIT` работали.
        if trimmed.eq_ignore_ascii_case("QUIT") {
            let _ = writeln!(stdout, "bye");
            break;
        }

        // Вся работа — здесь. execute разбирает строку и применяет её к store.
        let response = execute(&mut store, trimmed);

        // Печатаем ответ и тут же сбрасываем буфер, чтобы он появился сразу (важно, когда
        // ввод идёт из пайпа). `let _ =` намеренно глотает ошибку записи в stdout: если
        // вывод закрыт, падать смысла нет.
        let _ = writeln!(stdout, "{response}");
        let _ = stdout.flush();
    }
}

/// Короткая шапка при старте — подсказка пользователю, что это за программа.
fn print_banner(out: &mut impl Write) {
    let _ = writeln!(out, "kvstore REPL. Команды: SET GET DEL COUNT KEYS SAVE LOAD QUIT.");
    let _ = writeln!(out, "Пример: SET name Алиса  |  GET name  |  QUIT для выхода.");
    let _ = out.flush();
}
