#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>

static FILE g_stdin = {0, 0, 0, 0, 0};
static FILE g_stdout = {1, 0, 0, 0, 0};
static FILE g_stderr = {2, 0, 0, 0, 0};

FILE *stdin = &g_stdin;
FILE *stdout = &g_stdout;
FILE *stderr = &g_stderr;

static int parse_mode(const char *mode, int *flags) {
  if (!mode || !flags) {
    return -1;
  }
  int plus = 0;
  for (const char *p = mode; *p; p++) {
    if (*p == '+') {
      plus = 1;
    }
  }
  switch (mode[0]) {
    case 'r':
      *flags = plus ? O_RDWR : O_RDONLY;
      return 0;
    case 'w':
      *flags = (plus ? O_RDWR : O_WRONLY) | O_CREAT | O_TRUNC;
      return 0;
    case 'a':
      *flags = (plus ? O_RDWR : O_WRONLY) | O_CREAT | O_APPEND;
      return 0;
    default:
      return -1;
  }
}

FILE *fdopen(int fd, const char *mode) {
  (void)mode;
  if (fd < 0) {
    errno = EBADF;
    return NULL;
  }
  FILE *f = (FILE *)malloc(sizeof(*f));
  if (!f) {
    errno = ENOMEM;
    return NULL;
  }
  f->fd = fd;
  f->eof = 0;
  f->err = 0;
  f->ungot = 0;
  f->ungot_ch = 0;
  return f;
}

FILE *fopen(const char *path, const char *mode) {
  int flags = 0;
  if (parse_mode(mode, &flags) != 0) {
    errno = EINVAL;
    return NULL;
  }
  int fd = open(path, flags);
  if (fd < 0) {
    return NULL;
  }
  return fdopen(fd, mode);
}

FILE *freopen(const char *path, const char *mode, FILE *stream) {
  if (!stream) {
    errno = EINVAL;
    return NULL;
  }
  FILE *f = fopen(path, mode);
  if (!f) {
    return NULL;
  }
  int fd = f->fd;
  f->fd = stream->fd;
  stream->fd = fd;
  stream->eof = 0;
  stream->err = 0;
  if (f != stdin && f != stdout && f != stderr) {
    free(f);
  }
  return stream;
}

int fclose(FILE *stream) {
  if (!stream) {
    errno = EINVAL;
    return -1;
  }
  int fd = stream->fd;
  int ret = close(fd);
  if (stream != stdin && stream != stdout && stream != stderr) {
    free(stream);
  }
  return ret;
}

size_t fread(void *ptr, size_t size, size_t nmemb, FILE *stream) {
  if (!stream || !ptr || size == 0 || nmemb == 0) {
    return 0;
  }
  size_t total = size * nmemb;
  ssize_t n = read(stream->fd, ptr, total);
  if (n < 0) {
    stream->err = 1;
    return 0;
  }
  if (n == 0) {
    stream->eof = 1;
    return 0;
  }
  return (size_t)n / size;
}

size_t fwrite(const void *ptr, size_t size, size_t nmemb, FILE *stream) {
  if (!stream || !ptr || size == 0 || nmemb == 0) {
    return 0;
  }
  size_t total = size * nmemb;
  ssize_t n = write(stream->fd, ptr, total);
  if (n < 0) {
    stream->err = 1;
    return 0;
  }
  return (size_t)n / size;
}

int fgetc(FILE *stream) {
  if (!stream) {
    errno = EINVAL;
    return EOF;
  }
  if (stream->ungot) {
    stream->ungot = 0;
    return (int)stream->ungot_ch;
  }
  unsigned char ch = 0;
  ssize_t n = read(stream->fd, &ch, 1);
  if (n == 1) {
    return (int)ch;
  }
  if (n == 0) {
    stream->eof = 1;
    return EOF;
  }
  stream->err = 1;
  return EOF;
}

int getc_unlocked(FILE *stream) {
  return fgetc(stream);
}

int getc(FILE *stream) {
  return fgetc(stream);
}

int getchar(void) {
  return fgetc(stdin);
}

char *fgets(char *s, int size, FILE *stream) {
  if (!s || size <= 0) {
    return NULL;
  }
  int i = 0;
  for (; i < size - 1; i++) {
    int c = fgetc(stream);
    if (c == EOF) {
      break;
    }
    s[i] = (char)c;
    if (c == '\n') {
      i++;
      break;
    }
  }
  if (i == 0) {
    return NULL;
  }
  s[i] = '\0';
  return s;
}

char *fgets_unlocked(char *s, int size, FILE *stream) {
  return fgets(s, size, stream);
}

int ungetc(int c, FILE *stream) {
  if (!stream || c == EOF) {
    return EOF;
  }
  if (stream->ungot) {
    return EOF;
  }
  stream->ungot = 1;
  stream->ungot_ch = (unsigned char)c;
  stream->eof = 0;
  return c;
}

int fputc(int c, FILE *stream) {
  if (!stream) {
    errno = EINVAL;
    return EOF;
  }
  unsigned char ch = (unsigned char)c;
  ssize_t n = write(stream->fd, &ch, 1);
  if (n != 1) {
    stream->err = 1;
    return EOF;
  }
  return (int)ch;
}

int putc_unlocked(int c, FILE *stream) {
  return fputc(c, stream);
}

int putchar_unlocked(int c) {
  return fputc(c, stdout);
}

int putc(int c, FILE *stream) {
  return fputc(c, stream);
}

int fputs(const char *s, FILE *stream) {
  if (!stream || !s) {
    errno = EINVAL;
    return EOF;
  }
  size_t len = strlen(s);
  ssize_t n = write(stream->fd, s, len);
  if (n < 0) {
    stream->err = 1;
    return EOF;
  }
  return (int)n;
}

int fputs_unlocked(const char *s, FILE *stream) {
  return fputs(s, stream);
}

int fseek(FILE *stream, long offset, int whence) {
  if (!stream) {
    errno = EINVAL;
    return -1;
  }
  off_t ret = lseek(stream->fd, (off_t)offset, whence);
  if (ret < 0) {
    stream->err = 1;
    return -1;
  }
  stream->eof = 0;
  return 0;
}

int fseeko(FILE *stream, off_t offset, int whence) {
  return fseek(stream, (long)offset, whence);
}

long ftell(FILE *stream) {
  if (!stream) {
    errno = EINVAL;
    return -1;
  }
  off_t ret = lseek(stream->fd, 0, SEEK_CUR);
  if (ret < 0) {
    stream->err = 1;
    return -1;
  }
  return (long)ret;
}

int fgetpos(FILE *stream, fpos_t *pos) {
  if (!pos) {
    errno = EINVAL;
    return -1;
  }
  long off = ftell(stream);
  if (off < 0) {
    return -1;
  }
  *pos = (fpos_t)off;
  return 0;
}

int fsetpos(FILE *stream, const fpos_t *pos) {
  if (!pos) {
    errno = EINVAL;
    return -1;
  }
  return fseek(stream, (long)*pos, SEEK_SET);
}

void rewind(FILE *stream) {
  if (!stream) {
    return;
  }
  (void)fseek(stream, 0, SEEK_SET);
  clearerr(stream);
}

int fflush(FILE *stream) {
  (void)stream;
  return 0;
}

int feof(FILE *stream) {
  return stream ? stream->eof : 0;
}

int ferror(FILE *stream) {
  return stream ? stream->err : 0;
}

int ferror_unlocked(FILE *stream) {
  return ferror(stream);
}

void clearerr(FILE *stream) {
  if (!stream) {
    return;
  }
  stream->err = 0;
  stream->eof = 0;
}

int fileno_unlocked(FILE *stream) {
  if (!stream) {
    errno = EINVAL;
    return -1;
  }
  return stream->fd;
}

int fileno(FILE *stream) {
  return fileno_unlocked(stream);
}

int remove(const char *path) {
  if (!path) {
    errno = EINVAL;
    return -1;
  }
  return unlink(path);
}

FILE *tmpfile(void) {
  char tmpl[] = "/tmp/tmpXXXXXX";
  int fd = mkstemp(tmpl);
  if (fd < 0) {
    return NULL;
  }
  return fdopen(fd, "w+");
}

void setbuf(FILE *stream, char *buf) {
  (void)stream;
  (void)buf;
}

int setvbuf(FILE *stream, char *buf, int mode, size_t size) {
  (void)stream;
  (void)buf;
  (void)mode;
  (void)size;
  return 0;
}

ssize_t getline(char **lineptr, size_t *n, FILE *stream) {
  if (!lineptr || !n || !stream) {
    errno = EINVAL;
    return -1;
  }
  if (!*lineptr || *n == 0) {
    *n = 128;
    *lineptr = (char *)malloc(*n);
    if (!*lineptr) {
      errno = ENOMEM;
      return -1;
    }
  }
  char *res = fgets(*lineptr, (int)*n, stream);
  if (!res) {
    return -1;
  }
  return (ssize_t)strlen(*lineptr);
}

int sscanf(const char *str, const char *fmt, ...) {
  (void)str;
  (void)fmt;
  return 0;
}

int fscanf(FILE *stream, const char *fmt, ...) {
  (void)stream;
  (void)fmt;
  return 0;
}

int scanf(const char *fmt, ...) {
  (void)fmt;
  return 0;
}

int vasprintf(char **strp, const char *fmt, va_list ap) {
  if (!strp) {
    return -1;
  }
  int size = 256;
  char *buf = (char *)malloc((size_t)size);
  if (!buf) {
    return -1;
  }
  va_list ap_copy;
  va_copy(ap_copy, ap);
  int n = vsnprintf(buf, (size_t)size, fmt, ap_copy);
  va_end(ap_copy);
  if (n < 0) {
    free(buf);
    return -1;
  }
  if (n >= size) {
    size = n + 1;
    char *next = (char *)realloc(buf, (size_t)size);
    if (!next) {
      free(buf);
      return -1;
    }
    buf = next;
    va_list ap_copy2;
    va_copy(ap_copy2, ap);
    n = vsnprintf(buf, (size_t)size, fmt, ap_copy2);
    va_end(ap_copy2);
    if (n < 0) {
      free(buf);
      return -1;
    }
  }
  *strp = buf;
  return n;
}

int rename(const char *oldpath, const char *newpath) {
  (void)oldpath;
  (void)newpath;
  errno = ENOSYS;
  return -1;
}
