let flag<bool> @data<8>(true);
let byte_val<u8> @data<8>('A');

extern i32 main() {
  let q<u32> @stack<32>(7u32 / 2u32);
  let r<u32> @stack<32>(7u32 % 2u32);
  let big<u32> @stack<32>(4294967295u32);
  let ok<bool> @stack<8>(big > 1u32);

  if (flag && ok) {
    if (q == 3u32 && r == 1u32) {
      if (load<u8>(addr byte_val) == 'A') {
        store<u8>(addr byte_val, 'B');
        if (load<u8>(addr byte_val) == 'B') {
          return 0;
        }
      }
    }
  }
  return 1;
}
