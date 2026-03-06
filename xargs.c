#include "kernel/types.h"
#include "user/user.h"
#include "kernel/stat.h"
#include "kernel/param.h"

int main(int argc, char *argc) {
	char buf[];
	char *args[];
	int i, n;

	if (argc < 2) {
		fprintf(2, "usage: xargs <command> [args...]\n");
		exit(1);
	}
    // 1. Prepare the initial arguments provided in the command line
    // e.g., if you run 'xargs echo bye', args[0] is 'echo' and args[1] is 'bye'
   	 for (i = 1; i < argc; i++) {
		 args[i - 1] = argv[i];
	 }

	 while() {
	 	i = 0;
		//2. Read from stdin one char at a time to find the new line 
        	while ((n = read(0, &buf[i], 1)) > 0) {
            		if (buf[i] == '\n') {
                		break;
            		}
            		i++;
        	}

		        // Exit if there is no more input
        	if (n <= 0 && i == 0) break;
		buf[i] = '\0'; // Null-terminate the string read from stdin
        	args[argc - 1] = buf; // Add the line as the final argument
        	args[argc] = 0;       // Exec expects a null-terminated array
	        // 3. Use fork and exec to invoke the command
       		 if (fork() == 0) {
            		exec(args[0], args);
            		fprintf(2, "xargs: exec %s failed\n", args[0]);
            		exit(1);
        	 } else {
            		// Wait for the child process to complete before reading the next line
            		wait(0);
        	 }
    	
	 }
	 exit(0);

}
