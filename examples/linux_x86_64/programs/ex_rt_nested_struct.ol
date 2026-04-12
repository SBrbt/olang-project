// Nested struct: field access and aggregate field assignment.
type Pair = struct { a: i64, b: i64 };
type Wrap = struct { p: Pair };

extern i32 main() {
  let pair<Pair> @stack<128>();
  let wrap<Wrap> @stack<128>();
  pair.a = 1i64;
  pair.b = 2i64;
  wrap.p = pair;
  if (wrap.p.a == 1i64 && wrap.p.b == 2i64) {
    return 0;
  }
  return 1;
}
