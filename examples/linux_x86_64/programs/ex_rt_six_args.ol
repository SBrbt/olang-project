i64 sum6(a: i64, b: i64, c: i64, d: i64, e: i64, f: i64) {
  return load[a] + load[b] + load[c] + load[d] + load[e] + load[f];
}

extern i32 main() {
  let s stack[64, sum6(1i64, 2i64, 3i64, 4i64, 5i64, 6i64)];
  if (load[s] == 21i64) {
    return 0;
  }
  return 1;
}
