// Test f32 arithmetic and comparisons.
extern i32 main() {
  let a<f32> @stack<32>(10.5f32);
  let b<f32> @stack<32>(3.5f32);
  let sum<f32> @stack<32>(a + b);
  let diff<f32> @stack<32>(a - b);
  let prod<f32> @stack<32>(a * b);
  let quot<f32> @stack<32>(a / b);

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
