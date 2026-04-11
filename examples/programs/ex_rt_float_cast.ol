// Test cast between float and integer types.
extern fn main() -> i32 {
  // i64 -> f64
  let a: f64 = cast<f64>(42i64);
  if (a < 41.0) { return 1; }
  if (a > 43.0) { return 2; }

  // f64 -> i64 (truncation)
  let b: i64 = cast<i64>(3.99);
  if (b != 3i64) { return 3; }

  // i32 -> f32
  let c: f32 = cast<f32>(100);
  if (c < 99.0f32) { return 4; }
  if (c > 101.0f32) { return 5; }

  // f32 -> i32 (truncation)
  let d: i32 = cast<i32>(7.9f32);
  if (d != 7) { return 6; }

  // f32 -> f64 widening
  let e: f32 = 1.5f32;
  let f: f64 = cast<f64>(e);
  if (f < 1.4) { return 7; }
  if (f > 1.6) { return 8; }

  // f64 -> f32 narrowing
  let g: f64 = 2.5;
  let h: f32 = cast<f32>(g);
  if (h < 2.4f32) { return 9; }
  if (h > 2.6f32) { return 10; }

  return 0;
}
