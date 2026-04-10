extern fn main() -> i32 {
  let x: i32 = 0;
  store<i32>(addr x, 99);
  if (load<i32>(addr x) == 99) {
    return 0;
  }
  return 1;
}
