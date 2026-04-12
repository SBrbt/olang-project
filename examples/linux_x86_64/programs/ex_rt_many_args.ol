i64 sum10(a: i64, b: i64, c: i64, d: i64, e: i64, f: i64, g: i64, h: i64, i: i64, j: i64) {
  return load<a> + load<b> + load<c> + load<d> + load<e> + load<f> + load<g> + load<h> + load<i> + load<j>;
}

extern i32 main() {
  let s<i64> @stack<64>(sum10(1i64, 2i64, 3i64, 4i64, 5i64, 6i64, 7i64, 8i64, 9i64, 10i64));
  if (load<s> == 55i64) {
    return 0;
  }
  return 1;
}
