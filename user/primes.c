#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

void printPrim(int p[2]);

int
main(int argc, char *argv[])
{
  int p[2];
  pipe(p);
  if (fork() == 0) {
    printPrim(p);
    exit(0);
  } else {
    close(p[0]);
    for (int i = 2; i <= 35; i++) {
      write(p[1], &i, sizeof(int));
    }
    close(p[1]);
  }
  wait(0);
  exit(0);
}

void
printPrim(int pl[2]) {
  close(pl[1]);
  int prime = 0;
  read(pl[0], &prime, sizeof(int));
  printf("prime %d\n", prime);
  int n = 0;
  int pr[2];
  int haschild = 0;
  while (read(pl[0], &n, sizeof(int))) {
    if (n % prime != 0) { // pass number to right neighbor
      if (!haschild) {
        pipe(pr);
        int pid = fork();
        if (!pid) {
          printPrim(pr);
          exit(0);
        } else {
          close(pr[0]);
          write(pr[1], &n, sizeof(int));
          haschild = 1;
        }
      } else {
        write(pr[1], &n, sizeof(int));
      }
    }
  }
  if (haschild) {
    close(pr[1]);
  }
  close(pl[0]);
  wait(0);
  exit(0);
}
