extern i32 main() {
  let i<i32> @stack<32>(0);
  while (true) {
    i = i + 1;
    if (i == 1) {
      continue;
    }
    if (i == 2) {
      break;
    }
    return 1;
  }
  if (i == 2) {
    return 0;
  }
  return 1;
}
