#include <dirent.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>

#define RESET_COLOR "\033[0m"
#define BLUE_COLOR "\033[34m"
#define LIGHT_GREEN_COLOR "\033[92m"
#define YELLOW_COLOR "\033[33m"

void print_with_color(struct dirent *entry) {
  char *typ = "f";
  char *icon = "";
  char *color = RESET_COLOR;

  if (entry->d_type == DT_DIR) {
    typ = "d";
    color = BLUE_COLOR;
    icon = "";
  } else if (entry->d_type == DT_REG) {
    color = LIGHT_GREEN_COLOR;
  } else if (entry->d_type == DT_LNK) {
    typ = "l";
    color = YELLOW_COLOR;
    icon = "";
  }

  printf("%s%s\t%s  %s%s\n", color, typ, icon, entry->d_name, RESET_COLOR);
}

void ls_dir(const char *path) {
  char *p = (path == NULL) ? "." : (char *)path;

  DIR *dir = opendir(p);
  if (dir == NULL) {
    perror("opendir");
    return;
  }

  // clear errno before readdir
  errno = 0;
  struct dirent *entry;
  while ((entry = readdir(dir)) != NULL) {
    // clear the director of the current and parent directory
    if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
      continue;
    }

    print_with_color(entry);
  }

  if (errno != 0) {
    perror("readdir");
  }

  closedir(dir);
}

int main(int argc, char *argv[]) {
  if (argc > 1) {
    ls_dir(argv[1]);
  } else {
    ls_dir(NULL);
  }

  return 0;
}
