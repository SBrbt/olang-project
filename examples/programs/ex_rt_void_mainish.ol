void via_return() {
  return;
}

void via_ret() {
  return;
}

extern i32 main() {
  via_return();
  via_ret();
  return 0;
}
