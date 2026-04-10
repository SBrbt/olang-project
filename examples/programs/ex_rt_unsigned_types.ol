let flag: bool = true;
let byte_val: u8 = 'A';

extern fn main() -> i32 {
  let q: u32 = 7u32 / 2u32;
  let r: u32 = 7u32 % 2u32;
  let big: u32 = 4294967295u32;
  let ok: bool = big > 1u32;

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
