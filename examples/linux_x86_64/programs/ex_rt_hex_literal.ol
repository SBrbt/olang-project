extern i32 main() {
  let x<i64> @stack<64>(0xFFi64);
  if (load<x> == 255i64) {
    return 0;
  }
  return 1;
}
