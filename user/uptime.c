#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int
main(int argc, char *argv[])
{
  // The uptime() system call returns the number of ticks
  int t = uptime();

  // Print the result to the console
  printf("uptime: %d ticks\n", t);

  // Exit the program successfully
  exit(0);
}
