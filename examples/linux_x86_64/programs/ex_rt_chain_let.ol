// Chained let: views built from an existing reference (innermost name next to rhs).
extern i32 main() {
  let base<i32> @stack<32>(42i32);
  let v<u32> <u32>addr base;
  let w<u32> let u<u32> v;
  if (load<u> != 42u32) { return 1; }
  if (load<w> != 42u32) { return 2; }
  return 0;
}
