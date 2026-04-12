i32 fib(n: i32) {
  if (load<n> < 2) {
    return load<n>;
  }
  let a<i32> @stack<32>(fib(load<n> - 1));
  return fib(load<n> - 2) + load<a>;
}

extern i32 main() {
  if (fib(10) == 55) {
    return 0;
  }
  return 1;
}
