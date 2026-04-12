extern i32 main() {
  let i<i32> @stack<32>(0);
  while (true) {
    store<i, load<i> + 1>;
    if (load<i> == 1) {
      continue;
    }
    if (load<i> == 2) {
      break;
    }
    return 1;
  }
  if (load<i> == 2) {
    return 0;
  }
  return 1;
}
