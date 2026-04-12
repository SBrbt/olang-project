extern i32 main() {
  let a<i64> @stack<64>(1000i64);
  let b<i32> @stack<32>(cast<i32>(a));
  if (b == 1000) {
    return 0;
  }
  return 1;
}
