#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/param.h"

// char* copyArgs(char *to[], char *from[], char *appendArg)
// {
//   while (*from != '\0') {
//     *to++ = *from++;
//   }
//   *to++ = appendArg;
//   *to = '\0';
// }

// int
// main(int argc, char *argv[])
// {
//   char buf[512];
//   char *p = buf;
//   int n;
//   while (n = read(0, p, sizeof buf))
//   {
//     p += n;
//     if (p > buf + 512) {
//       fprintf(2, "xargs: input is too long");
//       exit(1);
//     }
//   }
//   p = buf;
//   char *cmd; //TODO: parse cmd from buffer
//   int pid; 
//   pid = fork();
//   if (pid = 0) {
//     char *argv1[MAXARG];
//     copyArgs(argv1, argv + 1, cmd); //argv: [xargs grep hello]
//     exec(argv[1], argv1);
//     printf("exec failed!\n");
//     exit(1);
//   } else {
//     wait(0);
//   }
//   gets("", 1);
// }

int
main(int argc, char *argv[])
{
  

  const int BUFLEN = 512;
  char buf[BUFLEN];
  while (strlen(gets(buf, BUFLEN)) > 0) {
    buf[strlen(buf) - 1] = 0; // QUESTION: 2021/01/15 debug 了一上午，最后还是参考了别人的代码
    // printf("buf len: %d\n",strlen(buf));
    // for (int si = 0; buf[si]; si++) {
    //   if (buf[si] == ' ') {
    //     printf("find a \' \' ");
    //   }
    //   if (buf[si] == '\r') {
    //     printf("find a \' \' ");
    //   }
    //   printf("%c#", buf[si]);
    // }
    // printf("=========>\\n\n");

    if (fork() == 0) {
      char* argv1[MAXARG];
      // copy argv
      int argIdx = 0;
      for (int i = 1; argv[i]; i++) {
        argv1[argIdx++] = argv[i];
      }
      int i = 0;
      while (argIdx + 1 < MAXARG) {
        while (buf[i] && buf[i] == ' ') {
          ++i;
        }
        if (!buf[i]) {
          break;
        }
        argv1[argIdx++] = buf + i;
        while (buf[i] && buf[i] != ' ') {
          ++i;
        }
        if (!buf[i]) {
          //printf("[%s], [%d], [%d]\n", argv1[argIdx - 1], strlen(argv1[argIdx - 1]), argIdx - 1);
          break;
        } else {
          buf[i++] = '\0';
        }
        // printf("[%s], [%d], [%d]\n", argv1[argIdx - 1], strlen(argv1[argIdx - 1]), argIdx - 1);
      }
      argv1[argIdx] = '\0';
      exec(argv1[0], argv1);
      exit(0);
    }
    wait(0);
  }
  exit(0);
}
