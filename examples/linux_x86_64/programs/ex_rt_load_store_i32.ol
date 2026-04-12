extern i32 main() {
  let x<i32> @stack<32>(0);
  store<i32>(addr x, 99);
  if (load<i32>(addr x) == 99) {
    return 0;
  }
  return 1;
}
