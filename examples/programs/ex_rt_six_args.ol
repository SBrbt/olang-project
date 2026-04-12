i64 sum6(a: i64, b: i64, c: i64, d: i64, e: i64, f: i64) {
  return a + b + c + d + e + f;
}

extern i32 main() {
  let s<i64> @stack<64>(sum6(1i64, 2i64, 3i64, 4i64, 5i64, 6i64));
  if (s == 21i64) {
    return 0;
  }
  return 1;
}
