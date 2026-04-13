// typed <T> must not wrap addr[...] (ptr); use <T>x where x is ref
extern i32 main() {
  let x stack[32, 0i32];
  let y <u32>addr[x];
  return 0;
}
