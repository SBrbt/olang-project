// Test global variables with section attributes.
let MAGIC<i32> @rodata<32>(42);
let gcnt<i32> @data<32>(10);
let gscratch<i64> @bss<64>();

extern i32 main() {
  if (load<MAGIC> != 42) { return 1; }
  if (load<gcnt> != 10) { return 2; }
  if (load<gscratch> == 0i64) {
    return 0;
  }
  return 3;
}
