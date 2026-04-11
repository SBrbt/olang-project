// Test i8, i16, u16 types with arithmetic and comparisons.
extern fn main() -> i32 {
  let a: i8 = 42i8;
  let b: i8 = 10i8;
  let c: i16 = 1000i16;
  let d: i16 = 200i16;
  let e: u16 = 50000u16;
  let f: u16 = 15000u16;

  if (a + b == 52i8) {
    if (a - b == 32i8) {
      if (c + d == 1200i16) {
        if (e - f == 35000u16) {
          if (c > d) {
            if (a >= b) {
              return 0;
            }
          }
        }
      }
    }
  }
  return 1;
}
