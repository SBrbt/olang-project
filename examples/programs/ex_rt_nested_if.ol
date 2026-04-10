extern fn main() -> i32 {
  if (true) {
    if (false) {
      return 1;
    } else {
      if (true) {
        return 0;
      }
    }
  }
  return 2;
}
