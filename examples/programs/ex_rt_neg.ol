extern i32 main() {
  let a<i32> @stack<32>(-5);
  let b<i64> @stack<64>(-10i64);
  if (a == -5) {
    if (b == -10i64) {
      return 0;
    }
  }
  return 1;
}
