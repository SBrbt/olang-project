// Test global variables with section attributes.
let MAGIC<i32> @rodata<32>(42);
let gcnt<i32> @data<32>(10);
let gscratch<i64> @bss<64>();

extern i32 main() {
  if (MAGIC != 42) { return 1; }
  if (gcnt != 10) { return 2; }
  if (gscratch == 0i64) {
    return 0;
  }
  return 3;
}
