let flag data[8, true];
let byte_val data[8, 'A'];

extern i32 main() {
  let q stack[32, 7u32 / 2u32];
  let r stack[32, 7u32 % 2u32];
  let big stack[32, 4294967295u32];
  let ok stack[8, load[big] > 1u32];

  if (load[flag] && load[ok]) {
    if (load[q] == 3u32 && load[r] == 1u32) {
      if (load[byte_val] == 'A') {
        store[byte_val, 'B'];
        if (load[byte_val] == 'B') {
          return 0;
        }
      }
    }
  }
  return 1;
}
