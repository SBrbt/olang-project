// Indirect binding: find[…] with load[…] in the operand; inner must be ptr value; load/store use that view.
let mutcell data[32, 0];

extern i32 main() {
  let p stack[64, addr[mutcell]];
  let pv <i32>(find[(load[p])]);
  store[pv, 5];
  if (load[mutcell] != 5i32) {
    return 1;
  }
  if (load[pv] != 5i32) {
    return 2;
  }
  return 0;
}
