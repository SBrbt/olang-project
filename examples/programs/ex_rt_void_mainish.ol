fn via_return() -> void {
  return;
}

fn via_ret() -> void {
  return;
}

extern fn main() -> i32 {
  via_return();
  via_ret();
  return 0;
}
