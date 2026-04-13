// Nested struct: field access and aggregate field assignment.
type Pair = struct { a: i64, b: i64 };
type Wrap = struct { p: Pair };

extern i32 main() {
  let pair_raw stack[128];
  let pair <Pair> pair_raw;
  let wrap_raw stack[128];
  let wrap <Wrap> wrap_raw;
  store[pair.a, 1i64];
  store[pair.b, 2i64];
  store[wrap.p, pair];
  if (load[wrap.p.a] != 1i64) {
    return 1;
  }
  return 0;
}
