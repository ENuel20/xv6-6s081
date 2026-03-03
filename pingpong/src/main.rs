use libc::{
    close, fork, pipe, read, write, waitpid,
    clock_gettime, timespec, CLOCK_MONOTONIC,
    c_int,
};
use std::mem::MaybeUninit;
use std::ptr;

const ITERATIONS: usize = 1_000_000;

unsafe fn now_ns() -> i128 {
    let mut ts = MaybeUninit::<timespec>::uninit();

    if clock_gettime(CLOCK_MONOTONIC, ts.as_mut_ptr()) != 0 {
        panic!("clock_gettime failed");
    }

    let ts = ts.assume_init();

    (ts.tv_sec as i128) * 1_000_000_000 +
    (ts.tv_nsec as i128)
}

fn main() {
    unsafe {
        // pipe1: parent -> child
        let mut p1 = [0 as c_int; 2];
        // pipe2: child -> parent
        let mut p2 = [0 as c_int; 2];

        if pipe(p1.as_mut_ptr()) != 0 {
            panic!("pipe1 failed");
        }

        if pipe(p2.as_mut_ptr()) != 0 {
            panic!("pipe2 failed");
        }

        let pid = fork();

        if pid < 0 {
            panic!("fork failed");
        }

        if pid == 0 {
            // --- CHILD ---

            // Close unused ends
            close(p1[1]); // child doesn't write to pipe1
            close(p2[0]); // child doesn't read from pipe2

            let mut buf = [0u8; 1];

            for _ in 0..ITERATIONS {
                // read from parent
                if read(p1[0], buf.as_mut_ptr() as *mut _, 1) != 1 {
                    panic!("child read failed");
                }

                // write back
                if write(p2[1], buf.as_ptr() as *const _, 1) != 1 {
                    panic!("child write failed");
                }
            }

            close(p1[0]);
            close(p2[1]);

            std::process::exit(0);
        } else {
            // --- PARENT ---

            close(p1[0]); // parent doesn't read from pipe1
            close(p2[1]); // parent doesn't write to pipe2

            let mut buf = [0u8; 1];
            buf[0] = 1;

            let start = now_ns();

            for _ in 0..ITERATIONS {
                // send byte to child
                if write(p1[1], buf.as_ptr() as *const _, 1) != 1 {
                    panic!("parent write failed");
                }

                // wait for response
                if read(p2[0], buf.as_mut_ptr() as *mut _, 1) != 1 {
                    panic!("parent read failed");
                }
            }

            let end = now_ns();

            close(p1[1]);
            close(p2[0]);

            waitpid(pid, ptr::null_mut(), 0);

            let elapsed_ns = end - start;

            let exchanges_per_sec =
                (ITERATIONS as i128 * 1_000_000_000) / elapsed_ns;

            println!("Elapsed: {} ns", elapsed_ns);
            println!("Exchanges/sec: {}", exchanges_per_sec);
        }
    }
}
