fn bump() -> i32 {
  return 1;
}

extern fn main() -> i32 {
  if (bump() == 1) {
    return 0;
  }
  return 1;
}
