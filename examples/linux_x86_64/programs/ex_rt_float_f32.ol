// Test f32 arithmetic and comparisons.
extern i32 main() {
  let a stack[32, 10.5f32];
  let b stack[32, 3.5f32];
  let sum stack[32, load[a] + load[b]];
  let diff stack[32, load[a] - load[b]];
  let prod stack[32, load[a] * load[b]];
  let quot stack[32, load[a] / load[b]];

  if (load[sum] == 14.0f32) {
    if (load[diff] == 7.0f32) {
      if (load[prod] > 36.0f32) {
        if (load[quot] == 3.0f32) {
          return 0;
        }
      }
    }
  }
  return 1;
}
