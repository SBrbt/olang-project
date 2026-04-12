// Linux x86_64 syscall shims from libposix.kasm (OLang register ABI -> Linux syscall ABI).
// Linked by examples/olc together with libposix.oobj. Use via: #include "posix_abi.ol"
// (olc passes -I to the preprocessor for this directory.)
// Parameter names avoid OLang keywords (e.g. "addr" is reserved).

extern i64 posix_write(fd: i64, buf: ptr, n: i64);
extern i64 posix_read(fd: i64, buf: ptr, n: i64);
extern i64 posix_openat(dirfd: i32, path: ptr, flags: i32, mode: i32);
extern i64 posix_close(fd: i32);
extern i64 posix_getpid();
extern i64 posix_kill(pid: i64, sig: i32);
extern i64 posix_rt_sigprocmask(how: i32, sigset: ptr, oldset: ptr, sigsetsize: u64);
extern ptr posix_mmap(base: ptr, len: u64, prot: i32, mmap_flags: i32, fd: i32, offset: u64);
extern i64 posix_munmap(base: ptr, len: u64);
extern i64 posix_clock_gettime(clk_id: i32, tp: ptr);
extern i64 posix_gettimeofday(tv: ptr, tz: ptr);
extern i64 posix_fstat(fd: i32, statbuf: ptr);
extern i64 posix_getdents64(fd: i32, dirp: ptr, count: u64);
extern i64 posix_nanosleep(req: ptr, rem: ptr);
extern i64 posix_fork();
extern i64 posix_wait4(pid: i32, wstatus: ptr, options: i32, rusage: ptr);
extern void posix_exit(status: i32);
