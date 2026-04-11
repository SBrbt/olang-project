// Linux x86_64 syscall shims from libposix.kasm (OLang register ABI -> Linux syscall ABI).
// Linked by examples/olc together with libposix.oobj. Use via: #include "posix_abi.ol"
// (olc passes -I to the preprocessor for this directory.)
// Parameter names avoid OLang keywords (e.g. "addr" is reserved).

extern fn posix_write(fd: i64, buf: ptr, n: i64) -> i64;
extern fn posix_read(fd: i64, buf: ptr, n: i64) -> i64;
extern fn posix_openat(dirfd: i32, path: ptr, flags: i32, mode: i32) -> i64;
extern fn posix_close(fd: i32) -> i64;
extern fn posix_getpid() -> i64;
extern fn posix_kill(pid: i64, sig: i32) -> i64;
extern fn posix_rt_sigprocmask(how: i32, sigset: ptr, oldset: ptr, sigsetsize: u64) -> i64;
extern fn posix_mmap(base: ptr, len: u64, prot: i32, mmap_flags: i32, fd: i32, offset: u64) -> ptr;
extern fn posix_munmap(base: ptr, len: u64) -> i64;
extern fn posix_clock_gettime(clk_id: i32, tp: ptr) -> i64;
extern fn posix_gettimeofday(tv: ptr, tz: ptr) -> i64;
extern fn posix_fstat(fd: i32, statbuf: ptr) -> i64;
extern fn posix_getdents64(fd: i32, dirp: ptr, count: u64) -> i64;
extern fn posix_nanosleep(req: ptr, rem: ptr) -> i64;
extern fn posix_fork() -> i64;
extern fn posix_wait4(pid: i32, wstatus: ptr, options: i32, rusage: ptr) -> i64;
extern fn posix_exit(status: i32);
