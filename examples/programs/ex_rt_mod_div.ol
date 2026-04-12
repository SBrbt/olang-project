extern i32 main() {
  let q<i64> @stack<64>(17i64 / 5i64);
  let r<i64> @stack<64>(17i64 % 5i64);
  if (q == 3i64) {
    if (r == 2i64) {
      return 0;
    }
  }
  return 1;
}
