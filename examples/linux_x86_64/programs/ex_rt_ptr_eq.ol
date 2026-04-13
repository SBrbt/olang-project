// ptr equality and inequality (only == / != are defined for ptr).
extern i32 main() {
  let a stack[32, 1];
  let b stack[32, 2];
  let pa stack[64, addr[a]];
  let pb stack[64, addr[b]];
  let pa2 stack[64, addr[a]];
  if (load[pa] != load[pa2]) {
    return 1;
  }
  if (load[pa] == load[pb]) {
    return 2;
  }
  return 0;
}
