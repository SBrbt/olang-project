// Test + on unsigned types and bool == !=.
extern i32 main() {
  let a<u32> @stack<32>(100u32);
  let b<u32> @stack<32>(200u32);
  let s<u32> @stack<32>(a + b);
  if (s != 300u32) { return 1; }

  let c<u64> @stack<64>(50u64);
  let d<u64> @stack<64>(75u64);
  let t<u64> @stack<64>(c + d);
  if (t != 125u64) { return 2; }

  // bool == !=
  let x<bool> @stack<8>(true);
  let y<bool> @stack<8>(true);
  let z<bool> @stack<8>(false);
  if (x == y) {} else { return 3; }
  if (x != z) {} else { return 4; }

  return 0;
}
