// ls.c - A modern ls clone with colors, icons, -l, -a, and sorting

#include <dirent.h>
#include <grp.h>
#include <locale.h>
#include <pwd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <termios.h>
#include <time.h>
#include <unistd.h>

#define MAX_PATH 8192

// ANSI Colors
#define RESET "\033[0m"
#define LIGHT_BLUE "\033[94m"
#define LIGHT_GREEN "\033[92m"
#define CYAN "\033[36m"
#define LIGHT_MAGENTA "\033[95m"
#define LIGHT_YELLOW "\033[93m"
#define RED "\033[31m"
#define WHITE "\033[37m"
#define LIGHT_BLUE "\033[94m"

// Icon selection
const char *get_icon(const struct stat *st, const char *name) {
  if (S_ISDIR(st->st_mode))
    return " ";
  if (S_ISLNK(st->st_mode))
    return " ";

  const char *ext = strrchr(name, '.');
  if (ext) {
    if (strcmp(ext, ".c") == 0)
      return " ";
    if (strcmp(ext, ".md") == 0)
      return " ";
    if (strcmp(ext, ".json") == 0)
      return "{}";
    if (strcmp(ext, ".py") == 0)
      return " ";
    if (strcmp(ext, ".sh") == 0)
      return " ";
    if (strcmp(ext, ".txt") == 0)
      return " ";
    if (strcmp(ext, ".go") == 0)
      return " ";
    if (strcmp(ext, ".java") == 0)
      return " ";
    if (strcmp(ext, ".toml") == 0 || strcmp(ext, ".yaml") == 0)
      return " ";
    if (strcmp(ext, ".html") == 0)
      return " ";
    if (strcmp(ext, ".css") == 0)
      return " ";
    if (strcmp(ext, ".js") == 0)
      return " ";
    if (strcmp(ext, ".ts") == 0)
      return " ";
    if (strcmp(ext, ".vue") == 0)
      return " ";
    if (strcmp(ext, ".sql") == 0)
      return " ";
    if (strcmp(ext, ".png") == 0 || strcmp(ext, ".jpg") == 0 ||
        strcmp(ext, ".jpeg") == 0 || strcmp(ext, ".gif") == 0) {
      // Image files
      return " ";
    }
  }

  // if (access(name, X_OK) == 0) return " ";

  // Default icon for unknown types
  return " ";
}

// Color selection
const char *get_color(const struct stat *st, const char *name) {
  if (S_ISDIR(st->st_mode))
    return LIGHT_BLUE;
  if (S_ISLNK(st->st_mode))
    return CYAN;
  if (access(name, X_OK) == 0)
    return LIGHT_GREEN;
  const char *ext = strrchr(name, '.');
  if (ext && strcmp(ext, ".md") == 0)
    return LIGHT_MAGENTA;
  return LIGHT_YELLOW;
}

// Permissions
void get_permissions(mode_t mode, char *perm_str) {
  perm_str[0] = S_ISDIR(mode)    ? 'd'
                : S_ISLNK(mode)  ? 'l'
                : S_ISCHR(mode)  ? 'c'
                : S_ISBLK(mode)  ? 'b'
                : S_ISFIFO(mode) ? 'p'
                : S_ISSOCK(mode) ? 's'
                                 : '-';
  perm_str[1] = (mode & S_IRUSR) ? 'r' : '-';
  perm_str[2] = (mode & S_IWUSR) ? 'w' : '-';
  perm_str[3] = (mode & S_IXUSR) ? 'x' : '-';
  perm_str[4] = (mode & S_IRGRP) ? 'r' : '-';
  perm_str[5] = (mode & S_IWGRP) ? 'w' : '-';
  perm_str[6] = (mode & S_IXGRP) ? 'x' : '-';
  perm_str[7] = (mode & S_IROTH) ? 'r' : '-';
  perm_str[8] = (mode & S_IWOTH) ? 'w' : '-';
  perm_str[9] = (mode & S_IXOTH) ? 'x' : '-';
  perm_str[10] = '\0';
}

void format_time(time_t rawtime, char *buf, size_t len) {
  struct tm *tm_info = localtime(&rawtime);
  strftime(buf, len, "%b %d %H:%M:%S %Y", tm_info);
}

void human_size(off_t size, char *buf, size_t len) {
  const char *units[] = {"B", "K", "M", "G", "T"};
  int i = 0;
  double sz = size;
  while (sz >= 1024 && i < 4) {
    sz /= 1024;
    ++i;
  }
  snprintf(buf, len, "%.1f%s", sz, units[i]);
}

int cmp_by_name(const void *a, const void *b) {
  const char **fa = (const char **)a;
  const char **fb = (const char **)b;
  return strcmp(*fa, *fb);
}

int get_terminal_width() {
  struct winsize w;
  if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &w) == -1)
    return 80;
  return w.ws_col;
}

void list_dir(const char *path, int detailed, int show_hidden, int recursive,
              int level) {
  DIR *dir = opendir(path);
  if (!dir) {
    perror("opendir");
    return;
  }

  struct dirent *entry;
  char *files[10240];
  int count = 0;
  size_t maxlen = 0;

  while ((entry = readdir(dir)) != NULL) {
    if (!show_hidden && entry->d_name[0] == '.') {
      continue;
    }

    char *name = NULL;
    if (entry->d_type == DT_DIR) {
      size_t len = strlen(entry->d_name) + 2; // +1 for '/' +1 for '\0'
      name = malloc(len);
      sprintf(name, "%s/", entry->d_name);
    } else {
      name = strdup(entry->d_name);
    }

    files[count] = strdup(name);
    size_t len = strlen(entry->d_name);
    if (len > maxlen) {
      maxlen = len;
    }

    count++;
  }
  closedir(dir);

  if (count == 0) {
    printf("  %sNothing to show%s\n", LIGHT_YELLOW, RESET);
    return;
  }

  qsort(files, count, sizeof(char *), cmp_by_name);

  if (detailed) {
    // ls -l
    for (int i = 0; i < count; ++i) {
      char fullpath[MAX_PATH];
      snprintf(fullpath, sizeof(fullpath), "%s/%s", path, files[i]);

      struct stat st;
      if (lstat(fullpath, &st) == -1) {
        perror("stat");
        free(files[i]);
        continue;
      }

      const char *icon = get_icon(&st, fullpath);
      const char *color = get_color(&st, fullpath);

      char perm[11], timebuf[32], sizebuf[16];
      get_permissions(st.st_mode, perm);
      format_time(st.st_mtime, timebuf, sizeof(timebuf));
      human_size(st.st_size, sizebuf, sizeof(sizebuf));

      struct passwd *pw = getpwuid(st.st_uid);
      struct group *gr = getgrgid(st.st_gid);
      const char *user = pw ? pw->pw_name : "?";
      const char *group = gr ? gr->gr_name : "?";

      printf("  %s%-11s %-8s %-8s %-8s %-8s  %s %s %s\n", color, perm, user,
             group, sizebuf, timebuf, icon, files[i], RESET);
      free(files[i]);
    }
  } else if (!recursive) {
    // just ls command
    int term_width = get_terminal_width();
    int col_width = (int)maxlen + 6; // icon + spacing
    int cols = term_width / col_width;
    if (cols < 1) {
      // Ensure at least one column
      cols = 1;
    }

    for (int i = 0; i < count; ++i) {
      char fullpath[MAX_PATH];
      snprintf(fullpath, sizeof(fullpath), "%s/%s", path, files[i]);

      struct stat st;
      if (lstat(fullpath, &st) == -1) {
        perror("stat");
        free(files[i]);
        continue;
      }

      const char *icon = get_icon(&st, fullpath);
      const char *color = get_color(&st, fullpath);

      printf("  %s%s %-*s%s", color, icon, (int)maxlen + 1, files[i], RESET);

      if ((i + 1) % cols == 0) {
        printf("\n");
      }

      free(files[i]);
    }
    if (count % cols != 0) {
      printf("\n");
    }

    return;
  }

  if (recursive) {
    // ls -t
    for (int i = 0; i < count; ++i) {
      char fullpath[MAX_PATH];
      snprintf(fullpath, sizeof(fullpath), "%s/%s", path, files[i]);
      struct stat st;
      if (lstat(fullpath, &st) == -1) {
        perror("stat");
        continue;
      }

      const char *icon = get_icon(&st, fullpath);
      const char *color = get_color(&st, fullpath);

      if (strcmp(files[i], ".") == 0 && strcmp(files[i], "..") == 0) {
        // Skip current and parent directories
        continue;
      }

      if (S_ISDIR(st.st_mode)) {
        // 树状连接线显示
        for (int j = 0; j < level; j++) {
          printf("    ");
        }
        printf("  %s└── %s%s%s %s%s\n", LIGHT_BLUE, RESET, color, icon,
               files[i], RESET);
        list_dir(fullpath, detailed, show_hidden, recursive, level + 1);
      } else {
        // current file
        for (int j = 0; j < level; j++) {
          printf("    ");
        }
        printf("  %s└── %s%s%s %s%s\n", LIGHT_BLUE, RESET, color, icon,
               files[i], RESET);
      }

      free(files[i]);
    }
  }
}

int main(int argc, char *argv[]) {
  setlocale(LC_ALL, "");
  const char *path = ".";
  int detailed = 0, show_hidden = 0, recursive = 0;

  for (int i = 1; i < argc; i++) {
    if (strcmp(argv[i], "-l") == 0) {
      detailed = 1;
    } else if (strcmp(argv[i], "-a") == 0) {
      show_hidden = 1;
    } else if (strcmp(argv[i], "-t") == 0) {
      recursive = 1;
      printf("\n");
    } else {
      path = argv[i];
    }
  }

  if (recursive) {
    // -t should not show detailed info
    detailed = 0;
    show_hidden = 0;
  }

  list_dir(path, detailed, show_hidden, recursive, 0);
  return 0;
}
