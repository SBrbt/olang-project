// Array whose element type is a user-defined struct.
type Row = struct { a: i32, b: i32 };
type Tab = array<Row, 2>;

extern i32 main() {
  let t<Tab> @stack<128>();
  store<t[0].a, 1i32>;
  store<t[0].b, 2i32>;
  store<t[1].a, 10i32>;
  store<t[1].b, 20i32>;
  if (load<t[0].a> + load<t[1].b> != 21i32) {
    return 1;
  }
  return 0;
}
