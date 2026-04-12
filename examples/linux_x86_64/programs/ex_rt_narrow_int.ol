// Test i8, i16, u16 types with arithmetic and comparisons.
extern i32 main() {
  let a<i8> @stack<8>(42i8);
  let b<i8> @stack<8>(10i8);
  let c<i16> @stack<16>(1000i16);
  let d<i16> @stack<16>(200i16);
  let e<u16> @stack<16>(50000u16);
  let f<u16> @stack<16>(15000u16);

  if (load<a> + load<b> == 52i8) {
    if (load<a> - load<b> == 32i8) {
      if (load<c> + load<d> == 1200i16) {
        if (load<e> - load<f> == 35000u16) {
          if (load<c> > load<d>) {
            if (load<a> >= load<b>) {
              return 0;
            }
          }
        }
      }
    }
  }
  return 1;
}
