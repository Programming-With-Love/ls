#define _GNU_SOURCE

#include <dirent.h>
#include <errno.h>
#include <grp.h>
#include <pwd.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>

#define RESET "\033[0m"
#define RED "\033[31m"
#define GREEN "\033[32m"
#define YELLOW "\033[33m"
#define BLUE "\033[34m"
#define MAGENTA "\033[35m"
#define CYAN "\033[36m"

#define STR_LEN 1024

void get_per(mode_t mode, char *perm_str) {
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

void format_time(time_t rawtime, char *time_str, size_t len) {
  struct tm *timeinfo = localtime(&rawtime);
  strftime(time_str, len, "%b %d %H:%M %Y", timeinfo);
}

void human_readable_size(off_t size, char *buf, size_t bufsize) {
  const char *units[] = {"B", "K", "M", "G", "T", "P"};
  int unit_index = 0;
  double s = (double)size;

  while (s >= 1024 && unit_index < 5) {
    s /= 1024;
    unit_index++;
  }

  snprintf(buf, bufsize, "%.1f%s", s, units[unit_index]);
}

const char *get_extension(const char *filename) {
  const char *dot = strrchr(filename, '.');
  return (!dot || dot == filename) ? "" : dot + 1;
}

void print_with_color(struct dirent *entry, const char *path) {
  struct stat statbuf;
  if (lstat(path, &statbuf) == -1) {
    perror("stat");
    return;
  }

  // permissions
  char perm[11];
  get_per(statbuf.st_mode, perm);

  // user and group names
  struct passwd *pw = getpwuid(statbuf.st_uid);
  const char *user = pw ? pw->pw_name : "unknown";

  struct group *gr = getgrgid(statbuf.st_gid);
  const char *group = gr ? gr->gr_name : "unknown";

  // updated time
  char timebuf[32];
  format_time(statbuf.st_mtime, timebuf, sizeof(timebuf));

  // fize size
  char sizebuf[16];
  human_readable_size(statbuf.st_size, sizebuf, sizeof(sizebuf));

  char *icon = "";
  char *color = GREEN;

  if S_ISDIR (statbuf.st_mode) {
    color = BLUE;
    icon = "";
  } else if S_ISLNK (statbuf.st_mode) {
    color = YELLOW;
    icon = "";
  } else {
    const char *ext = get_extension(entry->d_name);
    if (strcmp(ext, "c") == 0) {
      // c
      icon = "";
      color = YELLOW;
    } else if (strcmp(ext, "md") == 0) {
      icon = "";
      color = MAGENTA;
    } else if (strcmp(ext, "json") == 0) {
      icon = "{}";
      color = YELLOW;
    }
  }

  printf("%s%s %-8s %-8s %-8s  %-8s \t%s  %s%s\n", color, perm, user, group,
         sizebuf, timebuf, icon, entry->d_name, RESET);
}

void ls_dir(const char *path) {
  char *dir_p = (path == NULL) ? "." : (char *)path;

  DIR *dir = opendir(dir_p);
  if (dir == NULL) {
    perror("opendir");
    return;
  }

  // clear errno before readdir
  errno = 0;
  struct dirent *entry;
  char fullpath[STR_LEN];

  while ((entry = readdir(dir)) != NULL) {
    // clear the director of the current and parent directory
    if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
      continue;
    }

    snprintf(fullpath, sizeof(fullpath), "%s/%s", dir_p, entry->d_name);
    print_with_color(entry, fullpath);
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
