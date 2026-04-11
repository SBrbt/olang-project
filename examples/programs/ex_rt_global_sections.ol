// Test global variables with section attributes.
@rodata let MAGIC: i32 = 42;
@data let gcnt: i32 = 10;
@bss let gscratch: i64;

extern fn main() -> i32 {
  if (MAGIC != 42) { return 1; }
  if (gcnt != 10) { return 2; }
  if (gscratch == 0i64) {
    return 0;
  }
  return 3;
}
