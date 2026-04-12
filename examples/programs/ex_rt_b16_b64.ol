// Test b16 and b64 types with bitwise ops, arithmetic, and literals.
extern i32 main() {
  // b16
  let a<b16> @stack<16>(255b16);
  let b<b16> @stack<16>(15b16);
  let and16<b16> @stack<16>(a & b);
  if (and16 != 15b16) { return 1; }
  let shl16<b16> @stack<16>(1b16 << 4b16);
  if (shl16 != 16b16) { return 2; }
  let sum16<b16> @stack<16>(a + b);
  if (sum16 != 270b16) { return 3; }

  // b64
  let c<b64> @stack<64>(1000b64);
  let d<b64> @stack<64>(7b64);
  let or64<b64> @stack<64>(c | d);
  if (or64 != 1007b64) { return 4; }
  let xor64<b64> @stack<64>(c ^ c);
  let zero64<b64> @stack<64>(reinterpret<b64>(0u64));
  if (xor64 != zero64) { return 5; }
  let prod64<b64> @stack<64>(c * 3b64);
  if (prod64 != 3000b64) { return 6; }

  return 0;
}
