// Рандомизированные property-тесты для m05_structs.
// Используют детерминированный ГПСЧ без внешних крейтов — прогон воспроизводим.

use m05_structs::*;

// маленький детерминированный ГПСЧ (xorshift64*), без внешних крейтов — воспроизводимо.
// Используй ЭТОТ helper во всех property-тестах модуля. Сид ФИКСИРОВАННЫЙ (hardcode), чтобы прогон был детерминированным.
fn next_u64(state: &mut u64) -> u64 {
    let mut x = *state;
    x ^= x >> 12;
    x ^= x << 25;
    x ^= x >> 27;
    *state = x;
    x.wrapping_mul(0x2545F4914F6CDD1D)
}
// целое в диапазоне [lo, hi] включительно (держи диапазон скромным, чтобы оракул не переполнял i64)
fn in_range(state: &mut u64, lo: i64, hi: i64) -> i64 {
    let span = (hi - lo + 1) as u64;
    lo + (next_u64(state) % span) as i64
}

// -----------------------------------------------------------------------
// Point — property tests
// -----------------------------------------------------------------------

/// distance_to(self, self) == 0 для любой точки.
#[test]
fn prop_distance_to_self_is_zero() {
    let mut rng: u64 = 0xDEAD_BEEF_1234_5678;
    for _ in 0..1000 {
        let x = in_range(&mut rng, -10_000, 10_000) as f64;
        let y = in_range(&mut rng, -10_000, 10_000) as f64;
        let p = Point::new(x, y);
        assert_eq!(
            p.distance_to(&p),
            0.0,
            "distance to self must be 0 for {:?}",
            p
        );
    }
}

/// distance_to симметрична: a.distance_to(&b) == b.distance_to(&a).
#[test]
fn prop_distance_symmetry() {
    let mut rng: u64 = 0xABCD_EF01_2345_6789;
    for _ in 0..1000 {
        let ax = in_range(&mut rng, -10_000, 10_000) as f64;
        let ay = in_range(&mut rng, -10_000, 10_000) as f64;
        let bx = in_range(&mut rng, -10_000, 10_000) as f64;
        let by = in_range(&mut rng, -10_000, 10_000) as f64;
        let a = Point::new(ax, ay);
        let b = Point::new(bx, by);
        assert_eq!(
            a.distance_to(&b),
            b.distance_to(&a),
            "distance not symmetric: {:?} <-> {:?}",
            a,
            b
        );
    }
}

/// distance_to всегда >= 0.
#[test]
fn prop_distance_non_negative() {
    let mut rng: u64 = 0x1111_2222_3333_4444;
    for _ in 0..1000 {
        let ax = in_range(&mut rng, -10_000, 10_000) as f64;
        let ay = in_range(&mut rng, -10_000, 10_000) as f64;
        let bx = in_range(&mut rng, -10_000, 10_000) as f64;
        let by = in_range(&mut rng, -10_000, 10_000) as f64;
        let a = Point::new(ax, ay);
        let b = Point::new(bx, by);
        assert!(
            a.distance_to(&b) >= 0.0,
            "negative distance for {:?} -> {:?}",
            a,
            b
        );
    }
}

/// distance_to совпадает с независимым оракулом (inline sqrt формулой).
#[test]
fn prop_distance_oracle() {
    let mut rng: u64 = 0xFEED_FACE_CAFE_BABE;
    for _ in 0..1000 {
        let ax = in_range(&mut rng, -1_000, 1_000) as f64;
        let ay = in_range(&mut rng, -1_000, 1_000) as f64;
        let bx = in_range(&mut rng, -1_000, 1_000) as f64;
        let by = in_range(&mut rng, -1_000, 1_000) as f64;
        let a = Point::new(ax, ay);
        let b = Point::new(bx, by);
        let expected = ((ax - bx).powi(2) + (ay - by).powi(2)).sqrt();
        assert_eq!(
            a.distance_to(&b),
            expected,
            "oracle mismatch for {:?} -> {:?}",
            a,
            b
        );
    }
}

/// Point::new сохраняет переданные x и y в полях.
#[test]
fn prop_point_new_stores_fields() {
    let mut rng: u64 = 0x0102_0304_0506_0708;
    for _ in 0..1000 {
        let x = in_range(&mut rng, -10_000, 10_000) as f64;
        let y = in_range(&mut rng, -10_000, 10_000) as f64;
        let p = Point::new(x, y);
        assert_eq!(p.x, x, "x field mismatch");
        assert_eq!(p.y, y, "y field mismatch");
    }
}

/// Point clone даёт значение, равное оригиналу (round-trip через Clone).
#[test]
fn prop_point_clone_equals_original() {
    let mut rng: u64 = 0xC0FF_EE00_BEEF_0001;
    for _ in 0..500 {
        let x = in_range(&mut rng, -10_000, 10_000) as f64;
        let y = in_range(&mut rng, -10_000, 10_000) as f64;
        let a = Point::new(x, y);
        let b = a.clone();
        assert_eq!(a, b, "clone not equal to original for {:?}", a);
    }
}

/// PartialEq: две независимо созданные точки с одинаковыми полями равны.
#[test]
fn prop_point_eq_same_coords() {
    let mut rng: u64 = 0x9876_5432_10AB_CDEF;
    for _ in 0..500 {
        let x = in_range(&mut rng, -10_000, 10_000) as f64;
        let y = in_range(&mut rng, -10_000, 10_000) as f64;
        let a = Point::new(x, y);
        let b = Point::new(x, y);
        assert_eq!(a, b, "equal points should compare equal");
    }
}

/// PartialEq: точки с разными координатами не равны.
#[test]
fn prop_point_ne_different_coords() {
    let mut rng: u64 = 0xAAAA_BBBB_CCCC_DDDD;
    for _ in 0..500 {
        let x = in_range(&mut rng, -9_000, 9_000) as f64;
        let y = in_range(&mut rng, -9_000, 9_000) as f64;
        // offset гарантированно ненулевой
        let dx = in_range(&mut rng, 1, 500) as f64;
        let a = Point::new(x, y);
        let b = Point::new(x + dx, y);
        assert_ne!(a, b, "points with different x should not be equal");
    }
}

// -----------------------------------------------------------------------
// Rectangle — property tests
// -----------------------------------------------------------------------

/// area == width * height (оракул).
#[test]
fn prop_rectangle_area_oracle() {
    let mut rng: u64 = 0x1234_5678_9ABC_DEF0;
    for _ in 0..1000 {
        let w = in_range(&mut rng, 0, 10_000) as f64;
        let h = in_range(&mut rng, 0, 10_000) as f64;
        let r = Rectangle::new(w, h);
        assert_eq!(r.area(), w * h, "area oracle mismatch for {:?}", r);
    }
}

/// area всегда >= 0 для неотрицательных сторон.
#[test]
fn prop_rectangle_area_non_negative() {
    let mut rng: u64 = 0xDEAD_C0DE_1234_ABCD;
    for _ in 0..1000 {
        let w = in_range(&mut rng, 0, 10_000) as f64;
        let h = in_range(&mut rng, 0, 10_000) as f64;
        let r = Rectangle::new(w, h);
        assert!(r.area() >= 0.0, "area should be >= 0 for {:?}", r);
    }
}

/// is_square == (width == height).
#[test]
fn prop_is_square_oracle() {
    let mut rng: u64 = 0x5555_6666_7777_8888;
    for _ in 0..1000 {
        let w = in_range(&mut rng, 1, 10_000) as f64;
        let h = in_range(&mut rng, 1, 10_000) as f64;
        let r = Rectangle::new(w, h);
        let expected = w == h;
        assert_eq!(
            r.is_square(),
            expected,
            "is_square mismatch for {:?}",
            r
        );
    }
}

/// Квадрат (w == h) всегда is_square.
#[test]
fn prop_square_is_square() {
    let mut rng: u64 = 0x0F0F_0F0F_F0F0_F0F0;
    for _ in 0..1000 {
        let side = in_range(&mut rng, 1, 10_000) as f64;
        let r = Rectangle::new(side, side);
        assert!(r.is_square(), "equal sides should be square, got {:?}", r);
    }
}

/// Rectangle::new сохраняет width и height в полях.
#[test]
fn prop_rectangle_new_stores_fields() {
    let mut rng: u64 = 0xBEEF_CAFE_DEAD_0011;
    for _ in 0..1000 {
        let w = in_range(&mut rng, 0, 10_000) as f64;
        let h = in_range(&mut rng, 0, 10_000) as f64;
        let r = Rectangle::new(w, h);
        assert_eq!(r.width, w, "width field mismatch");
        assert_eq!(r.height, h, "height field mismatch");
    }
}

// -----------------------------------------------------------------------
// Color — property tests
// -----------------------------------------------------------------------

/// brightness == r as u32 + g as u32 + b as u32 (оракул).
#[test]
fn prop_color_brightness_oracle() {
    let mut rng: u64 = 0x7E57_C0DE_1337_BABE;
    for _ in 0..1000 {
        let r = (next_u64(&mut rng) % 256) as u8;
        let g = (next_u64(&mut rng) % 256) as u8;
        let b = (next_u64(&mut rng) % 256) as u8;
        let c = Color(r, g, b);
        let expected = r as u32 + g as u32 + b as u32;
        assert_eq!(
            c.brightness(),
            expected,
            "brightness oracle mismatch for Color({}, {}, {})",
            r,
            g,
            b
        );
    }
}

/// brightness всегда в диапазоне [0, 765] (0*3..=255*3).
#[test]
fn prop_color_brightness_range() {
    let mut rng: u64 = 0xF00D_FACE_0BAD_C0DE;
    for _ in 0..1000 {
        let r = (next_u64(&mut rng) % 256) as u8;
        let g = (next_u64(&mut rng) % 256) as u8;
        let b = (next_u64(&mut rng) % 256) as u8;
        let brightness = Color(r, g, b).brightness();
        assert!(
            brightness <= 765,
            "brightness {} out of [0, 765] for Color({}, {}, {})",
            brightness,
            r,
            g,
            b
        );
    }
}

/// Color::brightness для чёрного (0,0,0) == 0.
#[test]
fn prop_color_brightness_black_is_zero() {
    assert_eq!(Color(0, 0, 0).brightness(), 0);
}

/// Color::brightness для белого (255,255,255) == 765.
#[test]
fn prop_color_brightness_white_is_765() {
    assert_eq!(Color(255, 255, 255).brightness(), 765);
}

/// Color поля сохраняются при создании (round-trip через .0/.1/.2).
#[test]
fn prop_color_fields_stored() {
    let mut rng: u64 = 0xACE1_FACE_F00D_BABE;
    for _ in 0..500 {
        let r = (next_u64(&mut rng) % 256) as u8;
        let g = (next_u64(&mut rng) % 256) as u8;
        let b = (next_u64(&mut rng) % 256) as u8;
        let c = Color(r, g, b);
        assert_eq!(c.0, r, "Color.0 mismatch");
        assert_eq!(c.1, g, "Color.1 mismatch");
        assert_eq!(c.2, b, "Color.2 mismatch");
    }
}

/// Color PartialEq: два одинаково созданных Color равны.
#[test]
fn prop_color_eq_same_components() {
    let mut rng: u64 = 0x1717_1717_1717_1717;
    for _ in 0..500 {
        let r = (next_u64(&mut rng) % 256) as u8;
        let g = (next_u64(&mut rng) % 256) as u8;
        let b = (next_u64(&mut rng) % 256) as u8;
        let a = Color(r, g, b);
        let c = Color(r, g, b);
        assert_eq!(a, c, "same-component colors should be equal");
    }
}
