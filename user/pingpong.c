#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int main(int argc, char *argv[]) {
  int p[2];
  char buf[512];

  pipe(p);
  if (fork() == 0) {
    close(p[0]);
    printf("%d: received ping\n", getpid());
    write(p[1], "received pong\n", 14);
    close(p[1]);
    exit(0);
  } else {
    close(p[1]);
    read(p[0], buf, sizeof buf);
    printf("%d: %s", getpid(), buf);
    close(p[0]);
  }
  printf("parent execute statement\n");
  exit(0);
}
