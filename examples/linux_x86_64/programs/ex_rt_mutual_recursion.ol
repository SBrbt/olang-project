i32 is_even(n: i32) {
  if (n == 0) {
    return 1;
  }
  return is_odd(n - 1);
}

i32 is_odd(n: i32) {
  if (n == 0) {
    return 0;
  }
  return is_even(n - 1);
}

extern i32 main() {
  if (is_even(4) == 1) {
    if (is_odd(5) == 1) {
      return 0;
    }
  }
  return 1;
}
