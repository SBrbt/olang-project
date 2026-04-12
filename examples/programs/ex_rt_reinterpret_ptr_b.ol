// Test reinterpret between ptr<->b64 and i64<->ptr.
extern i32 main() {
  // ptr <-> b64
  let a<u64> @stack<64>(12345u64);
  let p<ptr> @stack<64>(reinterpret<ptr>(a));
  let b<b64> @stack<64>(reinterpret<b64>(p));
  let c<u64> @stack<64>(reinterpret<u64>(p));
  if (c != 12345u64) { return 1; }

  // i64 <-> ptr
  let d<i64> @stack<64>(99i64);
  let q<ptr> @stack<64>(reinterpret<ptr>(reinterpret<u64>(d)));
  let e<u64> @stack<64>(reinterpret<u64>(q));
  if (e != 99u64) { return 2; }

  return 0;
}
