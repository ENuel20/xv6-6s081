#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int main(int argc, char* argv[]){
	//create a buffer
	int p1[2];
	//create a pipe
	pipe(p);
	if (fork() == 0) {
		//child process
		close(p[1]);
		sieve(p[0])
	
	}else {
		//parent process
		close(p[0]);
		for(int i = 2, i<=35, i++ ) {
			write(p[1],&i, sizeof(int))
		}
		close(p[1]);
		wait(0);
	}
	exit(0);
}

int sieve(int left_neighbor){
	int b;

	if (read(left_neighbor, &b, sizeof(int))) <= 0 {
		close(left_neighbor);
		exit(0)
	}
	printf("prime %d\n", b);

	int c[2];
	pipe(c);

	if (fork() == 0) {
		//child
		close(c[1]);
		close(left_neighbor);
		sieve(c[0])


	}else {
		//parent
		int n;
		close(c[0]);

		while(1){
			if (read(left_neighbor, &n, sizeof(int))) > 0 {
				if (n % p) != 0 {
					write(c[1], &n, sizeof(int));
				} 
			}

		}
		close(left_neighbor)
		close(c[1]);
		wait(0);
		exit(0)

		
	}
	 

}
