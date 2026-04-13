// Same 4 bytes as i32 -1, read through a u32 binding via addr + <u32>.
extern i32 main() {
  let x stack[32, -1];
  let y <u32>x;
  if (load[y] == 4294967295u32) {
    return 0;
  }
  return 1;
}
