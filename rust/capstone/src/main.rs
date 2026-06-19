//! minidb REPL — интерактивная оболочка над хранилищем.
//!
//! Это РАБОЧИЙ бинарник (не стаб): он уже написан и компилируется. Запускается так:
//!
//! ```bash
//! cargo run -p minidb            # из каталога rust/
//! ```
//!
//! Цикл прост: читаем строку из stdin → отдаём её в [`minidb::execute`] → печатаем
//! ответ. Команда `QUIT` (регистронезависимо) или конец ввода (Ctrl-D) завершают цикл.
//! Пустые строки пропускаются.
//!
//! Пока тела в библиотеке — `todo!()`, REPL соберётся и запустится, но на первой же
//! команде упадёт с паникой `not yet implemented`. Это ожидаемо: реализуй методы
//! [`minidb::Database`] и функцию [`minidb::execute`], и REPL оживёт. Сам REPL менять
//! не нужно — он опирается только на публичный API библиотеки.

use minidb::Database;
use std::io::{self, BufRead, Write};

fn main() {
    let mut db = Database::new();

    let stdin = io::stdin();
    let mut stdout = io::stdout();

    // Приветствие и приглашение к вводу.
    println!("minidb REPL — введите команду (QUIT для выхода).");
    print_prompt(&mut stdout);

    // Читаем построчно. Когда ввод кончится (Ctrl-D / закрытый pipe), lines() выдаст
    // None и цикл завершится — REPL дружелюбен и к интерактивному вводу, и к pipe.
    for line in stdin.lock().lines() {
        let line = match line {
            Ok(l) => l,
            Err(_) => break, // ошибка чтения stdin — выходим
        };

        let trimmed = line.trim();

        // Пустые строки игнорируем, просто печатаем новое приглашение.
        if trimmed.is_empty() {
            print_prompt(&mut stdout);
            continue;
        }

        // QUIT (в любом регистре) — корректный выход.
        if trimmed.eq_ignore_ascii_case("QUIT") {
            println!("bye");
            return;
        }

        // Вся логика — в библиотеке. Бинарник лишь печатает её ответ.
        let response = minidb::execute(&mut db, trimmed);
        println!("{response}");

        print_prompt(&mut stdout);
    }
}

/// Печатает приглашение `minidb> ` без перевода строки и сбрасывает буфер вывода,
/// чтобы приглашение появилось ДО ввода пользователя.
fn print_prompt<W: Write>(out: &mut W) {
    let _ = write!(out, "minidb> ");
    let _ = out.flush();
}
