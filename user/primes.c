#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

// Helper function to handle each stage of the sieve pipeline
void sieve(int left_pipe_read) {
    int p;
    
    // Step A: Read the first number from the left neighbor; this is a prime
    if (read(left_pipe_read, &p, sizeof(int)) <= 0) {
        close(left_pipe_read);
        exit(0);
    }

    printf("prime %d\n", p); //

    int p_right[2];
    pipe(p_right); // Create a new pipe for the right neighbor

    if (fork() == 0) {
        // Child: Becomes the next filter in the pipeline
        close(p_right[1]); // Close unused write end
        close(left_pipe_read); // Child doesn't need the original left pipe
        sieve(p_right[0]);
    } else {
        // Parent: Filters multiples of p
        close(p_right[0]); // Close unused read end
        int n;
        
        // Step D: Read remaining numbers from left neighbor
        while (read(left_pipe_read, &n, sizeof(int)) > 0) {
            // If p does not divide n, send n to the right neighbor
            if (n % p != 0) {
                write(p_right[1], &n, sizeof(int)); //
            }
        }
        
        close(left_pipe_read);
        close(p_right[1]); // Closing write end tells child read() will return 0
        wait((int *) 0); // Wait for the child (and its descendants) to finish
        exit(0);
    }
}

int main(int argc, char *argv[]) {
    int p[2];
    pipe(p); // Create initial pipe

    if (fork() == 0) {
        close(p[1]); 
        sieve(p[0]);
    } else {
        close(p[0]);
        // Feed numbers 2 through 35 into the pipeline
        for (int i = 2; i <= 35; i++) {
            write(p[1], &i, sizeof(int)); //
        }
        close(p[1]); // Signal end of input
        wait((int *) 0); // Wait for entire pipeline to terminate
    }
    exit(0);
}
