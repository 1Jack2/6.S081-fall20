#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/fs.h"

void find(char *path, char *target) {
  int fd;
  struct stat st;
  struct dirent de;

  if ((fd = open(path, 0)) < 0) {
    fprintf(2, "find: cannot open %s\n", path);
    return;
  }

  if (fstat(fd, &st) < 0) {
    fprintf(2, "find: cannot stat %s\n", path);
    close(fd);
    return;
  }

  switch (st.type) {
    case T_FILE:;
      int len = strlen(path);
      int i;
      for (i = len - 1; i >= 0; --i) {
        if (path[i] == '/') {
          ++i;
          break;
        }
      }
      // printf("%s\n", path + i);
      if (!strcmp(path + i, target)) {
        printf("%s\n", path);
      }
      break;
    case T_DIR:
      // printf("find start path: %s\n", path);
      while (read(fd, &de, sizeof(de)) == sizeof(de)) {
        if (de.inum == 0)  // QUESTION
          continue;
        // printf("de{%d, %s}\n",de.inum, de.name);
        if (!(strcmp(de.name, ".") && strcmp(de.name, ".."))) {
          // printf("pass . ..\n");
          continue;
        }
        int len1 = strlen(path) + 1 + strlen(de.name) + 1;
        char path1[len1];
        strcpy(path1, path);
        path1[strlen(path)] = '/';
        strcpy(path1 + strlen(path) + 1, de.name);
        path1[len1 - 1] = '\0';
        find(path1, target);
      }
      break;
  }
  close(fd);
}

int main(int argc, char *argv[]) {
  if (argc < 3) {
    exit(1);
  }
  find(argv[1], argv[2]);
  exit(0);
}