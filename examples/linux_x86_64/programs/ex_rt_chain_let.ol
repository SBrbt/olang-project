// Chained let: views built from an existing reference (innermost name next to rhs).
extern i32 main() {
  let base stack[32, 42i32];
  let v <u32>base;
  let w let u (v);
  if (load[u] != 42u32) { return 1; }
  if (load[w] != 42u32) { return 2; }
  return 0;
}
