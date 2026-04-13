// find[...] with extra parentheses; ptr points at scalar cell (same as ex_rt_deref_lazy style).
extern i32 main() {
  let cell stack[32, 42i32];
  let p stack[64, addr[cell]];
  let v1 <i32>(find[(load[p])]);
  let v2 <i32>(find[((load[p]))]);
  if (load[v1] != 42i32) {
    return 1;
  }
  if (load[v2] != 42i32) {
    return 2;
  }
  return 0;
}
