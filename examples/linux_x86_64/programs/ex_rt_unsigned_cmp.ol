// Test all comparison operators on unsigned types.
extern i32 main() {
  let a<u32> @stack<32>(100u32);
  let b<u32> @stack<32>(200u32);

  if (load<a> < load<b>) {} else { return 1; }
  if (load<a> <= load<b>) {} else { return 2; }
  if (load<b> > load<a>) {} else { return 3; }
  if (load<b> >= load<a>) {} else { return 4; }
  if (load<a> == load<a>) {} else { return 5; }
  if (load<a> != load<b>) {} else { return 6; }
  if (load<a> <= load<a>) {} else { return 7; }
  if (load<a> >= load<a>) {} else { return 8; }

  return 0;
}
