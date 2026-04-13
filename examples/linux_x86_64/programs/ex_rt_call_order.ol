i64 two() { return 2i64; }
i64 three() { return 3i64; }
i64 mul(a: i64, b: i64) { return load[a] * load[b]; }

extern i32 main() {
  let p stack[64, mul(two(), three())];
  if (load[p] == 6i64) {
    return 0;
  }
  return 1;
}
