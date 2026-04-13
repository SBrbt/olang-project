// Value cast T(expr) does not include bool.
extern i32 main() {
  let x stack[32, i32(true)];
  return 0;
}
