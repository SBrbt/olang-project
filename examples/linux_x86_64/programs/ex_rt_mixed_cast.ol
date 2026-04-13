extern i32 main() {
  let a stack[64, 1000i64];
  let b stack[32, i32((load[a]))];
  if (load[b] == 1000) {
    return 0;
  }
  return 1;
}
