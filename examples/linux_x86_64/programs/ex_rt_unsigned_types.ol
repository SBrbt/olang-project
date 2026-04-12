let flag<bool> @data<8>(true);
let byte_val<u8> @data<8>('A');

extern i32 main() {
  let q<u32> @stack<32>(7u32 / 2u32);
  let r<u32> @stack<32>(7u32 % 2u32);
  let big<u32> @stack<32>(4294967295u32);
  let ok<bool> @stack<8>(load<big> > 1u32);

  if (load<flag> && load<ok>) {
    if (load<q> == 3u32 && load<r> == 1u32) {
      if (load<byte_val> == 'A') {
        store<byte_val, 'B'>;
        if (load<byte_val> == 'B') {
          return 0;
        }
      }
    }
  }
  return 1;
}
