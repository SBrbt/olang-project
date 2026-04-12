void fill(px: ptr) {
  let cell<i32> <i32>(find<(load<px>)>);
  store<cell, 7>;
  return;
}

extern i32 main() {
  let x<i32> @stack<32>(0);
  fill(addr x);
  if (load<x> == 7) {
    return 0;
  }
  return 1;
}
