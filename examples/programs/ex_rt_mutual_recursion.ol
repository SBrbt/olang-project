fn is_even(n: i32) -> i32 {
  if (n == 0) {
    return 1;
  }
  return is_odd(n - 1);
}

fn is_odd(n: i32) -> i32 {
  if (n == 0) {
    return 0;
  }
  return is_even(n - 1);
}

extern fn main() -> i32 {
  if (is_even(4) == 1) {
    if (is_odd(5) == 1) {
      return 0;
    }
  }
  return 1;
}
