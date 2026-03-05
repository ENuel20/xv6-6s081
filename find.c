#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/fs.h"

int main(int argc, char* argv[]) {
	if(argc != 3){
		printf("find {dir} {dir/file}");
	}

	find(argv[1], argv[2]);
	exit(0)
}

void find(char* path, char* target){
	char buf[512], *p;
	int fd;
	struct diren de;
	struct stat st;

	//open the current path
	if((fd = open(path, 0) < 0 )){
		fprintf(2, "find: cannot open %s\n ", path);
		return;
	}

	//get the metadata for the path
	if(fstat(fd, &st) < 0){
		fprint(2,"find : cannot stat %s\n"path);
		close(fd);
		return;
	}

	//find should only start if its a directory
	if(st.type != T_DIR){
		close(fd);
		return;
	}

	//prepare the path buffer (similar to ls.c)
	if(strlen(path) + 1 + DIRSIZ + 1 > sizeof buf){
		printf("find: path too long\n");
		close(fd);
		return;
	}
	strcpy(buf, path);
	p = buf + strlen(buf);
	*p++ = '/';

	//iterate through directories entries
	while(read(fd, &de, sizeof(de)) == sizeof(de)){
		if (de.inum == 0)
			continue;

		//Hint: Dont recurse into "." and ".."
		if(strcmp(de.name, ".") == 0 || strcmp(de.name, ".."))
			continue;
		//build the full path for the current entry
		memmove(p, de.name, DIRSIZ);
		p[DIRSIZ] = 0;

		//get metadata for the specific entry

		if(stat(buf, &st) < 0){
			printf("find: cannot stat %s\n", buf);
			continue;
		}
		
		//Handle based on type 
		if(st.type == T_DIR){
			//Recursively call find to decend into sub-directories
			find(buf, target)
		}else if(st.type == T_FILE) {
			//use strcmp() to check for name match
			if(strcmp(de.name, target) == 0){
				printf("%s\n", buf)
			}
		}
	}
	close(fd);


}
