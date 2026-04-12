// ptr equality and inequality (only == / != are defined for ptr).
extern i32 main() {
  let a<i32> @stack<32>(1);
  let b<i32> @stack<32>(2);
  let pa<ptr> @stack<64>(addr a);
  let pb<ptr> @stack<64>(addr b);
  let pa2<ptr> @stack<64>(addr a);
  if (pa != pa2) {
    return 1;
  }
  if (pa == pb) {
    return 2;
  }
  return 0;
}
