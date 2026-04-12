// Phase1 has no reassignment; countdown uses recursion (not a mutating while).
i32 down(n: i32) {
  if (n == 0) {
    return 0;
  }
  return down(n - 1);
}

extern i32 main() {
  return down(25);
}
