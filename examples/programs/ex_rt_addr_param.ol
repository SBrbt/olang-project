fn fill(px: ptr) -> void {
  store<i32>(px, 7);
  return;
}

extern fn main() -> i32 {
  let x: i32 = 0;
  fill(addr x);
  if (load<i32>(addr x) == 7) {
    return 0;
  }
  return 1;
}
