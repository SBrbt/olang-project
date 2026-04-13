// load/store through addr for several scalar widths (not only i32).
extern i32 main() {
  let x8 stack[8, -5i8];
  store[x8, 7i8];
  if (load[x8] != 7i8) {
    return 1;
  }

  let x16 stack[16, 1000i16];
  store[x16, 42i16];
  if (load[x16] != 42i16) {
    return 2;
  }

  let x64 stack[64, 123456789i64];
  store[x64, -1i64];
  if (load[x64] != -1i64) {
    return 3;
  }

  let u32v stack[32, 0xFFFFFFFFu32];
  store[u32v, 0u32];
  if (load[u32v] != 0u32) {
    return 4;
  }

  return 0;
}
