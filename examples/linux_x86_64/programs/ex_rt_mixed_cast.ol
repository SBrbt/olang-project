extern i32 main() {
  let a<i64> @stack<64>(1000i64);
  let b<i32> @stack<32>(i32<(load<a>)>);
  if (load<b> == 1000) {
    return 0;
  }
  return 1;
}
