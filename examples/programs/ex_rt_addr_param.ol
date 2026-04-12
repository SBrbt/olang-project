void fill(px: ptr) {
  store<i32>(px, 7);
  return;
}

extern i32 main() {
  let x<i32> @stack<32>(0);
  fill(addr x);
  if (load<i32>(addr x) == 7) {
    return 0;
  }
  return 1;
}
