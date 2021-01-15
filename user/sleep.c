#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int
main(int argc, char *argv[])
{
  if(argc <= 1){
    fprintf(2, "usage: sleep time");
    exit(1);
  }
  int cnt = atoi(argv[1]);
  // printf("sleep start: %d\n", cnt);
  sleep(cnt);
  // printf("sleep finished: %d\n", cnt);
  exit(0);
}
