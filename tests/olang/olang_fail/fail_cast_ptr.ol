// Value cast T(expr) does not produce ptr.
extern i32 main() {
  let x stack[64, ptr(0u64)];
  return 0;
}
