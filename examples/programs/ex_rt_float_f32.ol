// Test f32 arithmetic and comparisons.
extern fn main() -> i32 {
  let a: f32 = 10.5f32;
  let b: f32 = 3.5f32;
  let sum: f32 = a + b;
  let diff: f32 = a - b;
  let prod: f32 = a * b;
  let quot: f32 = a / b;

  if (sum == 14.0f32) {
    if (diff == 7.0f32) {
      if (prod > 36.0f32) {
        if (quot == 3.0f32) {
          return 0;
        }
      }
    }
  }
  return 1;
}
