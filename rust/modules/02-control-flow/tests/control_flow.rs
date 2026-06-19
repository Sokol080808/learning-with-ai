// Эти тесты трогать не нужно — это эталон поведения.
// Они описывают, КАК должны вести себя функции из src/lib.rs. Сейчас они падают,
// потому что тела стоят на `todo!()`. Реализуй функции — и красное станет зелёным.

use m02_control_flow::*;

#[test]
fn fizzbuzz_small() {
    assert_eq!(fizzbuzz(3), vec!["1", "2", "Fizz"]);
    assert_eq!(fizzbuzz(5), vec!["1", "2", "Fizz", "4", "Buzz"]);
}

#[test]
fn fizzbuzz_empty_for_zero() {
    let v: Vec<String> = fizzbuzz(0);
    assert!(v.is_empty());
}

#[test]
fn fizzbuzz_full_cycle_to_15() {
    let expected = vec![
        "1", "2", "Fizz", "4", "Buzz", "Fizz", "7", "8", "Fizz", "Buzz", "11", "Fizz", "13", "14",
        "FizzBuzz",
    ];
    assert_eq!(fizzbuzz(15), expected);
}

#[test]
fn fizzbuzz_length_matches_n() {
    assert_eq!(fizzbuzz(100).len(), 100);
}

#[test]
fn factorial_base_cases() {
    assert_eq!(factorial(0), 1);
    assert_eq!(factorial(1), 1);
}

#[test]
fn factorial_small() {
    assert_eq!(factorial(5), 120);
    assert_eq!(factorial(10), 3_628_800);
}

#[test]
fn factorial_large_fits_u64() {
    // 20! — крупнейший факториал, который ещё помещается в u64.
    assert_eq!(factorial(20), 2_432_902_008_176_640_000);
}

#[test]
fn fib_base_cases() {
    assert_eq!(fib(0), 0);
    assert_eq!(fib(1), 1);
    assert_eq!(fib(2), 1);
}

#[test]
fn fib_small() {
    assert_eq!(fib(10), 55);
    assert_eq!(fib(20), 6765);
}

#[test]
fn fib_large_fits_u64() {
    assert_eq!(fib(50), 12_586_269_025);
    assert_eq!(fib(90), 2_880_067_194_370_816_120);
}

#[test]
fn gcd_basic() {
    assert_eq!(gcd(48, 18), 6);
    assert_eq!(gcd(18, 48), 6); // порядок аргументов не влияет
    assert_eq!(gcd(17, 5), 1); // взаимно простые
}

#[test]
fn gcd_with_zero() {
    assert_eq!(gcd(0, 9), 9);
    assert_eq!(gcd(9, 0), 9);
    assert_eq!(gcd(0, 0), 0);
}

#[test]
fn gcd_bigger() {
    assert_eq!(gcd(270, 192), 6);
    assert_eq!(gcd(1071, 462), 21);
}

#[test]
fn classify_all_branches() {
    assert_eq!(classify(-3), "negative");
    assert_eq!(classify(0), "zero");
    assert_eq!(classify(7), "positive");
}

#[test]
fn classify_boundaries() {
    assert_eq!(classify(-1), "negative");
    assert_eq!(classify(1), "positive");
    assert_eq!(classify(i64::MIN), "negative");
    assert_eq!(classify(i64::MAX), "positive");
}
