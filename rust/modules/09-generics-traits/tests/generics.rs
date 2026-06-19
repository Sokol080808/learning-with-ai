// Эти тесты трогать не нужно — это эталон поведения.
//
// Здесь зафиксировано, КАК должен вести себя твой код. Запускай:
//   ./rust/run.sh 09
// Сейчас всё красное (тела — todo!()), твоя задача — сделать зелёным.

use m09_generics::*;

// ---------- my_max ----------

#[test]
fn my_max_first_bigger() {
    assert_eq!(my_max(9, 3), 9);
}

#[test]
fn my_max_second_bigger() {
    assert_eq!(my_max(3, 9), 9);
}

#[test]
fn my_max_equal_returns_value() {
    assert_eq!(my_max(7, 7), 7);
}

#[test]
fn my_max_works_with_floats() {
    assert_eq!(my_max(2.5_f64, 1.5_f64), 2.5);
}

#[test]
fn my_max_works_with_str() {
    // Строки сравниваются лексикографически: "banana" > "apple".
    assert_eq!(my_max("apple", "banana"), "banana");
}

#[test]
fn my_max_works_with_chars() {
    assert_eq!(my_max('a', 'z'), 'z');
}

// ---------- largest ----------

#[test]
fn largest_in_the_middle() {
    let v = [1, 5, 2, 9, 4];
    assert_eq!(largest(&v), 9);
}

#[test]
fn largest_single_element() {
    assert_eq!(largest(&[42]), 42);
}

#[test]
fn largest_first_is_biggest() {
    assert_eq!(largest(&[100, 1, 2, 3]), 100);
}

#[test]
fn largest_last_is_biggest() {
    assert_eq!(largest(&[1, 2, 3, 100]), 100);
}

#[test]
fn largest_with_negatives() {
    assert_eq!(largest(&[-5, -1, -9, -3]), -1);
}

#[test]
fn largest_with_floats() {
    let v = [3.1_f64, 2.2, 5.9, 0.4];
    assert_eq!(largest(&v), 5.9);
}

#[test]
fn largest_with_chars() {
    assert_eq!(largest(&['b', 'k', 'a', 'z', 'm']), 'z');
}

// ---------- trait Summary / Article ----------

#[test]
fn article_summarize_returns_title() {
    let a = Article {
        title: String::from("Rust 1.0 вышел"),
    };
    assert_eq!(a.summarize(), "Rust 1.0 вышел");
}

#[test]
fn article_summarize_another_title() {
    let a = Article {
        title: String::from("Обобщения — это просто"),
    };
    assert_eq!(a.summarize(), "Обобщения — это просто");
}

#[test]
fn article_default_preview_uses_summarize() {
    let a = Article {
        title: String::from("Новости дня"),
    };
    // preview не переопределён в Article — используется реализация по умолчанию.
    assert_eq!(a.preview(), "Читать дальше: Новости дня");
}

#[test]
fn summary_works_through_generic_bound() {
    // Обобщённая функция, принимающая ЛЮБОЙ тип с трейтом Summary.
    fn announce<T: Summary>(item: &T) -> String {
        item.preview()
    }

    let a = Article {
        title: String::from("Анонс"),
    };
    assert_eq!(announce(&a), "Читать дальше: Анонс");
}
