extern fn write(fd: i32, buf: ptr, count: u64) -> i64;
extern fn exit(code: i32);

type Int3 = array<i32, 3>;

extern fn main() -> i32 {
  let arr: Int3;
  // Initialize via index
  arr[0] = 10i32;
  arr[1] = 20i32;
  arr[2] = 30i32;
  // Sum and verify
  let sum: i32 = arr[0] + arr[1] + arr[2];
  // Return 0 if sum is 60 (expected), 1 otherwise
  if (sum == 60i32) {
    return 0;
  }
  return 1;
}
