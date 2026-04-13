extern i32 main() {
  let a stack[32, 42];
  let b stack[64, -10i64];
  if (load[a] != 42) {
    return 1;
  }
  if (load[b] != -10i64) {
    return 2;
  }
  return 0;
}
