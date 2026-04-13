// Nesting find[find[...]] is invalid (sema).
extern i32 main() {
  let x stack[64, "n"];
  let p <ptr>(find[(find[(load[x])])]);
  return 0;
}
