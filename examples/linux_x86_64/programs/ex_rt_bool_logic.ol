extern i32 main() {
  let a<bool> @stack<8>(true);
  let b<bool> @stack<8>(false);
  let ok<bool> @stack<8>(a && !b);
  let alt<bool> @stack<8>(b || ok);
  if (ok && alt) {
    return 0;
  }
  return 1;
}
