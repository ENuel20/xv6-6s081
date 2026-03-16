#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int main() {
    int p[2], c[2];
    char buf[1] = {'x'}; // Buffer to send/receive

    // Create two pipes: p for Parent->Child, c for Child->Parent
    pipe(p);
    pipe(c);

    if (fork() > 0) {
        // Parent process
        close(p[0]); // Close read end of parent pipe
        close(c[1]); // Close write end of child pipe

        write(p[1], buf, 1);
        
        if (read(c[0], buf, 1) == 1) {
            printf("%d: received pong\n", getpid());
        }

        close(p[1]);
        close(c[0]);
        wait(0);
        exit(0);
    } else {
        // Child process
        close(p[1]); // Close write end of parent pipe
        close(c[0]); // Close read end of child pipe

        if (read(p[0], buf, 1) == 1) {
            printf("%d: received ping\n", getpid());
        }

        write(c[1], buf, 1);

        close(p[0]);
        close(c[1]);
        exit(0);
    }
}
