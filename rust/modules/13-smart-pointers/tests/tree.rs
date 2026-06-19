// Тесты для задания 5 — разделяемое изменяемое дерево (TreeNode, Rc<RefCell<T>>).
// Файл является отдельным интеграционным крейтом; импортирует только публичный API.
//
// Структура файла:
//   1. Детерминированные примерные тесты (конкретный ввод → конкретный вывод).
//   2. Рандомизированные property-тесты (инварианты, оракул); ГПСЧ — xorshift64*,
//      фиксированный seed, без внешних крейтов.

use m13_smart_pointers::TreeNode;
use std::cell::RefCell;
use std::rc::Rc;

// ---------------------------------------------------------------------------
// Детерминированный ГПСЧ (xorshift64*) — без внешних крейтов.
// ---------------------------------------------------------------------------

fn next_u64(state: &mut u64) -> u64 {
    let mut x = *state;
    x ^= x >> 12;
    x ^= x << 25;
    x ^= x >> 27;
    *state = x;
    x.wrapping_mul(0x2545F4914F6CDD1D)
}

fn in_range(state: &mut u64, lo: i64, hi: i64) -> i64 {
    let span = (hi - lo + 1) as u64;
    lo + (next_u64(state) % span) as i64
}

// ===========================================================================
// Часть 1 — Детерминированные примерные тесты
// ===========================================================================

// --- TreeNode::new ---

#[test]
fn new_node_has_correct_value() {
    let node = TreeNode::new(42);
    assert_eq!(node.borrow().value, 42);
}

#[test]
fn new_node_has_no_children() {
    let node = TreeNode::new(7);
    assert!(node.borrow().children.is_empty());
}

#[test]
fn new_node_zero_value() {
    let node = TreeNode::new(0);
    assert_eq!(node.borrow().value, 0);
}

#[test]
fn new_node_negative_value() {
    let node = TreeNode::new(-100);
    assert_eq!(node.borrow().value, -100);
}

// --- add_child ---

#[test]
fn add_child_increases_children_count() {
    let parent = TreeNode::new(1);
    let child = TreeNode::new(2);
    TreeNode::add_child(&parent, child);
    assert_eq!(parent.borrow().children.len(), 1);
}

#[test]
fn add_multiple_children() {
    let parent = TreeNode::new(0);
    for i in 1..=5 {
        TreeNode::add_child(&parent, TreeNode::new(i));
    }
    assert_eq!(parent.borrow().children.len(), 5);
}

#[test]
fn added_child_has_correct_value() {
    let parent = TreeNode::new(10);
    let child = TreeNode::new(99);
    TreeNode::add_child(&parent, Rc::clone(&child));
    let stored_value = parent.borrow().children[0].borrow().value;
    assert_eq!(stored_value, 99);
}

// --- sum_tree ---

#[test]
fn sum_single_node() {
    let node = TreeNode::new(5);
    assert_eq!(TreeNode::sum_tree(&node), 5);
}

#[test]
fn sum_single_node_zero() {
    let node = TreeNode::new(0);
    assert_eq!(TreeNode::sum_tree(&node), 0);
}

#[test]
fn sum_single_node_negative() {
    let node = TreeNode::new(-7);
    assert_eq!(TreeNode::sum_tree(&node), -7);
}

#[test]
fn sum_root_with_two_children() {
    // root(1) → child(2), child(3)   → сумма = 1 + 2 + 3 = 6
    let root = TreeNode::new(1);
    TreeNode::add_child(&root, TreeNode::new(2));
    TreeNode::add_child(&root, TreeNode::new(3));
    assert_eq!(TreeNode::sum_tree(&root), 6);
}

#[test]
fn sum_three_level_tree() {
    //       10
    //      /  \
    //     3    4
    //    / \
    //   1   2
    // итого: 10 + 3 + 4 + 1 + 2 = 20
    let root = TreeNode::new(10);
    let left = TreeNode::new(3);
    TreeNode::add_child(&left, TreeNode::new(1));
    TreeNode::add_child(&left, TreeNode::new(2));
    TreeNode::add_child(&root, left);
    TreeNode::add_child(&root, TreeNode::new(4));
    assert_eq!(TreeNode::sum_tree(&root), 20);
}

#[test]
fn sum_chain_five_nodes() {
    // 1 → 2 → 3 → 4 → 5  (каждый следующий — единственный ребёнок)
    // сумма = 15
    let n5 = TreeNode::new(5);
    let n4 = TreeNode::new(4);
    let n3 = TreeNode::new(3);
    let n2 = TreeNode::new(2);
    let n1 = TreeNode::new(1);
    TreeNode::add_child(&n4, n5);
    TreeNode::add_child(&n3, n4);
    TreeNode::add_child(&n2, n3);
    TreeNode::add_child(&n1, n2);
    assert_eq!(TreeNode::sum_tree(&n1), 15);
}

#[test]
fn sum_tree_with_negatives() {
    // root(0) → child(-5), child(5)  → сумма = 0
    let root = TreeNode::new(0);
    TreeNode::add_child(&root, TreeNode::new(-5));
    TreeNode::add_child(&root, TreeNode::new(5));
    assert_eq!(TreeNode::sum_tree(&root), 0);
}

#[test]
fn sum_wide_tree_ten_leaves() {
    // Корень 100, десять листьев по 1; сумма = 110
    let root = TreeNode::new(100);
    for _ in 0..10 {
        TreeNode::add_child(&root, TreeNode::new(1));
    }
    assert_eq!(TreeNode::sum_tree(&root), 110);
}

// --- depth ---

#[test]
fn depth_single_node_is_one() {
    let node = TreeNode::new(0);
    assert_eq!(TreeNode::depth(&node), 1);
}

#[test]
fn depth_root_with_one_child_is_two() {
    let root = TreeNode::new(0);
    TreeNode::add_child(&root, TreeNode::new(1));
    assert_eq!(TreeNode::depth(&root), 2);
}

#[test]
fn depth_root_with_two_children_is_two() {
    let root = TreeNode::new(0);
    TreeNode::add_child(&root, TreeNode::new(1));
    TreeNode::add_child(&root, TreeNode::new(2));
    assert_eq!(TreeNode::depth(&root), 2);
}

#[test]
fn depth_three_level_chain() {
    // a → b → c    глубина = 3
    let a = TreeNode::new(1);
    let b = TreeNode::new(2);
    let c = TreeNode::new(3);
    TreeNode::add_child(&b, c);
    TreeNode::add_child(&a, b);
    assert_eq!(TreeNode::depth(&a), 3);
}

#[test]
fn depth_asymmetric_tree_uses_longest_branch() {
    //     root
    //    /    \
    //   a      b
    //   |
    //   c
    // Левая ветвь: depth=3, правая: depth=2 → итог 3
    let root = TreeNode::new(0);
    let a = TreeNode::new(1);
    let c = TreeNode::new(3);
    TreeNode::add_child(&a, c);
    TreeNode::add_child(&root, a);
    TreeNode::add_child(&root, TreeNode::new(2));
    assert_eq!(TreeNode::depth(&root), 3);
}

#[test]
fn depth_chain_of_five() {
    // 1 → 2 → 3 → 4 → 5   depth = 5
    let nodes: Vec<Rc<RefCell<TreeNode>>> = (1..=5).map(|v| TreeNode::new(v)).collect();
    for i in (0..4).rev() {
        TreeNode::add_child(&nodes[i], Rc::clone(&nodes[i + 1]));
    }
    assert_eq!(TreeNode::depth(&nodes[0]), 5);
}

#[test]
fn depth_wide_flat_tree_is_two() {
    // Корень + 20 листьев; все листья на одном уровне → depth = 2
    let root = TreeNode::new(0);
    for i in 0..20 {
        TreeNode::add_child(&root, TreeNode::new(i));
    }
    assert_eq!(TreeNode::depth(&root), 2);
}

// --- Комбинированные тесты ---

#[test]
fn sum_and_depth_empty_tree_is_leaf() {
    // «Пустое дерево» в нашей модели невозможно — минимум один узел.
    // Проверяем граничный случай: одиночный узел с нулём.
    let node = TreeNode::new(0);
    assert_eq!(TreeNode::sum_tree(&node), 0);
    assert_eq!(TreeNode::depth(&node), 1);
}

#[test]
fn shared_rc_child_is_counted_once_in_sum() {
    // Один и тот же Rc<RefCell<TreeNode>> добавлен как дочерний узел к родителю.
    // sum_tree обходит структуру дерева, не граф — ребёнок считается один раз.
    let root = TreeNode::new(10);
    let child = TreeNode::new(5);
    TreeNode::add_child(&root, Rc::clone(&child));
    // root(10) → child(5); сумма = 15
    assert_eq!(TreeNode::sum_tree(&root), 15);
}

// ===========================================================================
// Часть 2 — Рандомизированные property-тесты
// ===========================================================================

// --- Вспомогательные функции ---

/// Построить случайное «широкое» дерево глубиной 2:
/// корень со случайным числом листьев (0..=max_children) и случайными значениями.
/// Возвращает (root, expected_sum, expected_depth).
fn build_flat_tree(
    rng: &mut u64,
    max_children: usize,
    lo: i64,
    hi: i64,
) -> (Rc<RefCell<TreeNode>>, i64, usize) {
    let root_val = in_range(rng, lo, hi);
    let root = TreeNode::new(root_val);
    let n = next_u64(rng) as usize % (max_children + 1);
    let mut total = root_val;
    for _ in 0..n {
        let v = in_range(rng, lo, hi);
        TreeNode::add_child(&root, TreeNode::new(v));
        total += v;
    }
    let depth = if n == 0 { 1 } else { 2 };
    (root, total, depth)
}

/// Построить цепочку длиной `len` со значениями из `values`.
fn build_chain(values: &[i64]) -> Rc<RefCell<TreeNode>> {
    assert!(!values.is_empty());
    let nodes: Vec<_> = values.iter().map(|&v| TreeNode::new(v)).collect();
    for i in (0..nodes.len() - 1).rev() {
        TreeNode::add_child(&nodes[i], Rc::clone(&nodes[i + 1]));
    }
    Rc::clone(&nodes[0])
}

// --- Property-тесты ---

/// Свойство: sum_tree(leaf) == leaf.value для любого значения.
#[test]
fn prop_sum_single_node_equals_value() {
    let mut rng: u64 = 0xDEAD_BEEF_1357_2468;
    for _ in 0..800 {
        let v = in_range(&mut rng, -10_000, 10_000);
        let node = TreeNode::new(v);
        assert_eq!(
            TreeNode::sum_tree(&node),
            v,
            "sum_tree(leaf({v})) должна равняться {v}"
        );
    }
}

/// Свойство: depth(leaf) == 1 всегда.
#[test]
fn prop_depth_single_node_always_one() {
    let mut rng: u64 = 0xCAFE_BABE_8765_4321;
    for _ in 0..800 {
        let v = in_range(&mut rng, -10_000, 10_000);
        let node = TreeNode::new(v);
        assert_eq!(TreeNode::depth(&node), 1, "depth(leaf) должна быть 1");
    }
}

/// Оракул для плоского дерева: sum_tree == корень + сумма листьев.
#[test]
fn prop_flat_tree_sum_oracle() {
    let mut rng: u64 = 0x1111_AAAA_2222_BBBB;
    for _ in 0..600 {
        let (root, expected_sum, _) = build_flat_tree(&mut rng, 20, -500, 500);
        assert_eq!(
            TreeNode::sum_tree(&root),
            expected_sum,
            "sum_tree не совпадает с оракулом"
        );
    }
}

/// Оракул для плоского дерева: depth == 1 (нет детей) или 2 (есть хотя бы один ребёнок).
#[test]
fn prop_flat_tree_depth_oracle() {
    let mut rng: u64 = 0x3333_CCCC_4444_DDDD;
    for _ in 0..600 {
        let (root, _, expected_depth) = build_flat_tree(&mut rng, 20, -500, 500);
        assert_eq!(
            TreeNode::depth(&root),
            expected_depth,
            "depth плоского дерева должна быть {expected_depth}"
        );
    }
}

/// Оракул для цепочки длиной n: sum == сумма значений, depth == n.
#[test]
fn prop_chain_sum_and_depth() {
    let mut rng: u64 = 0x5555_EEEE_6666_FFFF;
    for _ in 0..600 {
        let n = in_range(&mut rng, 1, 20) as usize;
        let values: Vec<i64> = (0..n).map(|_| in_range(&mut rng, -100, 100)).collect();
        let root = build_chain(&values);
        let expected_sum: i64 = values.iter().sum();
        assert_eq!(
            TreeNode::sum_tree(&root),
            expected_sum,
            "sum_tree цепочки {:?} должна быть {expected_sum}",
            values
        );
        assert_eq!(
            TreeNode::depth(&root),
            n,
            "depth цепочки длиной {n} должна быть {n}"
        );
    }
}

/// Свойство: добавление ребёнка-листа увеличивает sum_tree ровно на leaf.value.
#[test]
fn prop_add_leaf_increases_sum_by_leaf_value() {
    let mut rng: u64 = 0x7777_0000_8888_1111;
    for _ in 0..600 {
        let root_val = in_range(&mut rng, -500, 500);
        let leaf_val = in_range(&mut rng, -500, 500);
        let n_extra = in_range(&mut rng, 0, 10) as usize;

        let root = TreeNode::new(root_val);
        // Добавляем несколько произвольных листьев
        for _ in 0..n_extra {
            let v = in_range(&mut rng, -100, 100);
            TreeNode::add_child(&root, TreeNode::new(v));
        }
        let sum_before = TreeNode::sum_tree(&root);

        // Добавляем ещё один лист
        TreeNode::add_child(&root, TreeNode::new(leaf_val));
        let sum_after = TreeNode::sum_tree(&root);

        assert_eq!(
            sum_after - sum_before,
            leaf_val,
            "добавление листа({leaf_val}) должно увеличить sum на {leaf_val}"
        );
    }
}

/// Свойство: добавление поддерева увеличивает depth не менее чем на 0
/// (а точнее: depth после >= depth до, т.к. поддерево добавляет хотя бы один уровень).
#[test]
fn prop_add_subtree_non_decreases_depth() {
    let mut rng: u64 = 0x9999_2222_AAAA_3333;
    for _ in 0..600 {
        let root = TreeNode::new(in_range(&mut rng, -100, 100));
        let n = in_range(&mut rng, 0, 5) as usize;
        for _ in 0..n {
            TreeNode::add_child(&root, TreeNode::new(in_range(&mut rng, -100, 100)));
        }
        let depth_before = TreeNode::depth(&root);

        // Добавляем поддерево глубиной >= 1
        let child = TreeNode::new(in_range(&mut rng, -100, 100));
        TreeNode::add_child(&root, child);
        let depth_after = TreeNode::depth(&root);

        assert!(
            depth_after >= depth_before,
            "depth не должна уменьшаться после добавления поддерева"
        );
    }
}

/// Свойство: new() создаёт узел с правильным value и без детей — многократно.
#[test]
fn prop_new_node_invariants() {
    let mut rng: u64 = 0xBBBB_4444_CCCC_5555;
    for _ in 0..800 {
        let v = in_range(&mut rng, -1_000_000_000, 1_000_000_000);
        let node = TreeNode::new(v);
        assert_eq!(node.borrow().value, v);
        assert!(node.borrow().children.is_empty());
        assert_eq!(TreeNode::sum_tree(&node), v);
        assert_eq!(TreeNode::depth(&node), 1);
    }
}

/// Свойство: sum всего дерева == сумма значений всех узлов (верифицируем через
/// собственный обход на стеке, не через рекурсию, чтобы это был настоящий оракул).
#[test]
fn prop_sum_tree_matches_iterative_oracle() {
    let mut rng: u64 = 0xDDDD_6666_EEEE_7777;
    for _ in 0..500 {
        // Строим случайное дерево итеративно и параллельно считаем сумму
        let root_val = in_range(&mut rng, -1_000, 1_000);
        let root = TreeNode::new(root_val);
        let mut expected_sum = root_val;
        let mut all_nodes = vec![Rc::clone(&root)];

        // Добавляем до 30 узлов, каждый — к случайному уже существующему
        let n_extra = in_range(&mut rng, 0, 30) as usize;
        for _ in 0..n_extra {
            let parent_idx = next_u64(&mut rng) as usize % all_nodes.len();
            let parent = Rc::clone(&all_nodes[parent_idx]);
            let v = in_range(&mut rng, -1_000, 1_000);
            let child = TreeNode::new(v);
            TreeNode::add_child(&parent, Rc::clone(&child));
            expected_sum += v;
            all_nodes.push(child);
        }

        assert_eq!(
            TreeNode::sum_tree(&root),
            expected_sum,
            "sum_tree должна совпадать с суммой всех добавленных значений"
        );
    }
}
