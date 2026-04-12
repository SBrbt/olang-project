i32 bump() {
  return 1;
}

extern i32 main() {
  if (bump() == 1) {
    return 0;
  }
  return 1;
}
