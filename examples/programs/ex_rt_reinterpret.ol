// Test reinterpret<T> for same-size type rebinding.
extern i32 main() {
  // i32 <-> u32 (both 4 bytes)
  let x<i32> @stack<32>(42);
  let y<u32> @stack<32>(reinterpret<u32>(x));
  if (y != 42u32) { return 1; }

  // u32 -> b32 (both 4 bytes)
  let z<b32> @stack<32>(reinterpret<b32>(y));
  if (z != 42b32) { return 2; }

  // b32 -> i32
  let w<i32> @stack<32>(reinterpret<i32>(z));
  if (w != 42) { return 3; }

  // i64 <-> u64 (both 8 bytes)
  let a<i64> @stack<64>(100i64);
  let b<u64> @stack<64>(reinterpret<u64>(a));
  if (b != 100u64) { return 4; }

  // u64 <-> ptr (both 8 bytes)
  let p<ptr> @stack<64>(reinterpret<ptr>(b));
  let c<u64> @stack<64>(reinterpret<u64>(p));
  if (c != 100u64) { return 5; }

  // i8 <-> u8 (both 1 byte)
  let s<i8> @stack<8>(7i8);
  let t<u8> @stack<8>(reinterpret<u8>(s));
  if (t != 7u8) { return 6; }

  return 0;
}
