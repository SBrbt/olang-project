// Lazy deref: `deref p as T` only binds; slot still holds ptr; reads/writes go through the pointer.
let mutcell<i32> @data<32>(0);

extern i32 main() {
  let p<ptr> @stack<64>(addr mutcell);
  deref p as i32;
  p = 5;
  if (mutcell != 5) {
    return 1;
  }
  if (p != 5) {
    return 2;
  }
  return 0;
}
