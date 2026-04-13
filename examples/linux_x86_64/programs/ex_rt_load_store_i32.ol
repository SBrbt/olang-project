extern i32 main() {
  let x stack[32, 0];
  store[x, 99];
  if (load[x] == 99) {
    return 0;
  }
  return 1;
}
