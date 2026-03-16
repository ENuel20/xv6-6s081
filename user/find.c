#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/fs.h"

void find(char *path, char *target) {
    char buf[512], *p;
    int fd;
    struct dirent de;
    struct stat st;

    // Open the current path
    if((fd = open(path, 0)) < 0){
        fprintf(2, "find: cannot open %s\n", path);
        return;
    }

    // Get metadata for the path
    if(fstat(fd, &st) < 0){
        fprintf(2, "find: cannot stat %s\n", path);
        close(fd);
        return;
    }

    // find should only start its search on a directory
    if(st.type != T_DIR){
        close(fd);
        return;
    }

    // Prepare the path buffer (similar to ls.c)
    if(strlen(path) + 1 + DIRSIZ + 1 > sizeof buf){
        printf("find: path too long\n");
        close(fd);
        return;
    }
    strcpy(buf, path);
    p = buf + strlen(buf);
    *p++ = '/';

    // Iterate through directory entries
    while(read(fd, &de, sizeof(de)) == sizeof(de)){
        if(de.inum == 0)
            continue;

        // HINT: Don't recurse into "." and ".."
        if(strcmp(de.name, ".") == 0 || strcmp(de.name, "..") == 0)
            continue;

        // Build the full path for the current entry
        memmove(p, de.name, DIRSIZ);
        p[DIRSIZ] = 0;

        // Get metadata for the specific entry
        if(stat(buf, &st) < 0){
            printf("find: cannot stat %s\n", buf);
            continue;
        }

        // Handle based on entry type
        if(st.type == T_DIR){
            // Recursively call find to descend into sub-directories
            find(buf, target);
        } else if(st.type == T_FILE){
            // Use strcmp() to check for name match
            if(strcmp(de.name, target) == 0){
                printf("%s\n", buf);
            }
        }
    }
    close(fd);
}

int main(int argc, char *argv[]) {
    if(argc != 3){
        fprintf(2, "Usage: find <path> <filename>\n");
        exit(1);
    }
    find(argv[1], argv[2]);
    exit(0);
}
