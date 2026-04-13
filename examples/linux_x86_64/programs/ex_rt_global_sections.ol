// Test global variables with section attributes.
let MAGIC rodata[32, 42];
let gcnt data[32, 10];
let gscratch data[64, 0i64];

extern i32 main() {
  if (load[MAGIC] != 42) { return 1; }
  if (load[gcnt] != 10) { return 2; }
  if (load[gscratch] == 0i64) {
    return 0;
  }
  return 3;
}
