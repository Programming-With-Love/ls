// fancy_ls.c - A modern ls clone with colors, icons, -l, -a, and sorting

#include <dirent.h>
#include <grp.h>
#include <locale.h>
#include <pwd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

#define MAX_PATH 1024

// ANSI Colors
#define RESET "\033[0m"
#define BLUE "\033[34m"
#define GREEN "\033[32m"
#define CYAN "\033[36m"
#define MAGENTA "\033[35m"
#define YELLOW "\033[33m"
#define RED "\033[31m"
#define WHITE "\033[37m"

// Icon selection
const char *get_icon(const struct stat *st, const char *name) {
  if (S_ISDIR(st->st_mode)) {
    return "📁";
  }
  if (S_ISLNK(st->st_mode)) {
    return "🔗";
  }
  if (access(name, X_OK) == 0) {
    return "🚀";
  }
  const char *ext = strrchr(name, '.');
  if (ext) {
    if (strcmp(ext, ".c") == 0) {
      return "🧠";
    }
    if (strcmp(ext, ".md") == 0) {
      return "📝";
    }
    if (strcmp(ext, ".json") == 0) {
      return "{}";
    }
  }
  return "📄";  // 📄
}

// Color selection
const char *get_color(const struct stat *st, const char *name) {
  if (S_ISDIR(st->st_mode)) {
    return BLUE;
  }
  if (S_ISLNK(st->st_mode)) {
    return CYAN;
  }
  if (access(name, X_OK) == 0) {
    return GREEN;
  }
  const char *ext = strrchr(name, '.');
  if (ext && strcmp(ext, ".md") == 0) {
    return MAGENTA;
  }
  return YELLOW;
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
  strftime(buf, len, "%Y-%m-%d %H:%M", tm_info);
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

void list_dir(const char *path, int detailed, int show_hidden) {
  DIR *dir = opendir(path);
  if (!dir) {
    perror("opendir");
    return;
  }

  struct dirent *entry;
  char *files[4096];
  int count = 0;

  while ((entry = readdir(dir)) != NULL) {
    if (!show_hidden && entry->d_name[0] == '.') continue;
    files[count++] = strdup(entry->d_name);
  }
  closedir(dir);

  qsort(files, count, sizeof(char *), cmp_by_name);

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

    if (detailed) {
      char perm[11], timebuf[32], sizebuf[16];
      get_permissions(st.st_mode, perm);
      format_time(st.st_mtime, timebuf, sizeof(timebuf));
      human_size(st.st_size, sizebuf, sizeof(sizebuf));

      struct passwd *pw = getpwuid(st.st_uid);
      struct group *gr = getgrgid(st.st_gid);
      const char *user = pw ? pw->pw_name : "?";
      const char *group = gr ? gr->gr_name : "?";

      printf("%s%-11s %-8s %-8s %8s %s %s %s%s\n", color, perm, user, group,
             sizebuf, timebuf, icon, files[i], RESET);
    } else {
      printf("%s%s %s%s    ", color, icon, files[i], RESET);
    }
    free(files[i]);
  }
}

int main(int argc, char *argv[]) {
  setlocale(LC_ALL, "");
  const char *path = ".";
  int detailed = 0, show_hidden = 0;

  for (int i = 1; i < argc; i++) {
    if (strcmp(argv[i], "-l") == 0)
      detailed = 1;
    else if (strcmp(argv[i], "-a") == 0)
      show_hidden = 1;
    else
      path = argv[i];
  }

  list_dir(path, detailed, show_hidden);
  return 0;
}
