// Aggregate value copy: assignment copies bits; later mutation of source does not affect dest.
type Pair = struct { a: i64, b: i64 };

extern i32 main() {
  let x<Pair> @stack<128>();
  let y<Pair> @stack<128>();
  x.a = 1i64;
  x.b = 2i64;
  y = x;
  x.b = 9i64;
  if (y.a == 1i64 && y.b == 2i64) {
    return 0;
  }
  return 1;
}
