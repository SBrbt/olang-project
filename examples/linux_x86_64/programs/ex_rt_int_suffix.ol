extern i32 main() {
  let a stack[32, 123i32];
  let b stack[64, 456i64];
  if (load[a] == 123) {
    if (load[b] == 456i64) {
      return 0;
    }
  }
  return 1;
}
