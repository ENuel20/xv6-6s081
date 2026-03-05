use std::env;
use std::process;
use libc;

fn main() {
    // Collect command line arguments
    let args: Vec<String> = env::args().collect();

    // Check if exactly one argument is provided (argc == 2)
    if args.len() != 2 {
        eprintln!("Usage: sleep seconds");
        process::exit(1);
    }

    // Parse the argument into a number
    let seconds: u32 = match args[1].parse() {
        Ok(n) => n,
        Err(_) => {
            eprintln!("Error: Invalid number of seconds");
            process::exit(1);
        }
    };

    unsafe {
        // POSIX sleep takes seconds as an unsigned int
        libc::sleep(seconds);
    }

    process::exit(0);
}
