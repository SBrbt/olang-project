// ptr <-> b64 / u64 / i64 views via <T>addr multi-bindings (same bits, different element types).
extern i32 main() {
  let a<u64> @stack<64>(12345u64);
  let p<ptr> <ptr>addr a;
  let b<b64> <b64>addr p;
  let c<u64> <u64>addr p;
  if (load<c> != 12345u64) { return 1; }
  if (load<b> != 12345b64) { return 3; }

  let d<i64> @stack<64>(99i64);
  let q<ptr> <ptr>addr d;
  let e<u64> <u64>addr q;
  if (load<e> != 99u64) { return 2; }

  return 0;
}
