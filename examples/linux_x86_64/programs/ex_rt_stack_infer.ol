// Width inferred from initializer type: stack[expr], data[expr], rodata[expr].
let gv rodata[32, 7u32];

extern i32 main() {
  let a stack[32, 42i32];
  let b stack[64, 100i64];
  if (load[a] != 42i32) {
    return 1;
  }
  if (load[b] != 100i64) {
    return 2;
  }
  if (load[gv] != 7u32) {
    return 3;
  }
  return 0;
}
