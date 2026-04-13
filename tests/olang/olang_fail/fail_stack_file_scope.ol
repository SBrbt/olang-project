// Plain stack[...] is not valid after global let (use data / bss / ...).
let x stack[32, 0i32];

extern i32 main() {
  return 0i32;
}
