extern i32 main() {
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
