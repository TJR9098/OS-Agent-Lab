#pragma once

#include <sys/types.h>
#include <stddef.h>
#include <stdarg.h>

typedef struct FILE {
  int fd;
  int eof;
  int err;
  int ungot;
  unsigned char ungot_ch;
} FILE;

typedef long fpos_t;

#define EOF (-1)
#define BUFSIZ 1024
#define FILENAME_MAX 256
#define L_tmpnam 20
#define _IOFBF 0
#define _IOLBF 1
#define _IONBF 2

extern FILE *stdin;
extern FILE *stdout;
extern FILE *stderr;

int printf(const char *fmt, ...);
int vprintf(const char *fmt, va_list ap);
int dprintf(int fd, const char *fmt, ...);
int fprintf(FILE *stream, const char *fmt, ...);
int vfprintf(FILE *stream, const char *fmt, va_list ap);
int fscanf(FILE *stream, const char *fmt, ...);
int scanf(const char *fmt, ...);
int sprintf(char *buf, const char *fmt, ...);
int vsprintf(char *buf, const char *fmt, va_list ap);
int snprintf(char *buf, size_t size, const char *fmt, ...);
int vsnprintf(char *buf, size_t size, const char *fmt, va_list ap);
int vasprintf(char **strp, const char *fmt, va_list ap);
int puts(const char *s);
int putchar(int c);

FILE *fopen(const char *path, const char *mode);
FILE *fdopen(int fd, const char *mode);
int fclose(FILE *stream);
size_t fread(void *ptr, size_t size, size_t nmemb, FILE *stream);
size_t fwrite(const void *ptr, size_t size, size_t nmemb, FILE *stream);
int fgetc(FILE *stream);
int getc_unlocked(FILE *stream);
int getc(FILE *stream);
int getchar(void);
char *fgets(char *s, int size, FILE *stream);
char *fgets_unlocked(char *s, int size, FILE *stream);
int ungetc(int c, FILE *stream);
int fputc(int c, FILE *stream);
int putc_unlocked(int c, FILE *stream);
int fputs(const char *s, FILE *stream);
int fputs_unlocked(const char *s, FILE *stream);
int putchar_unlocked(int c);
int putc(int c, FILE *stream);
int fseek(FILE *stream, long offset, int whence);
int fseeko(FILE *stream, off_t offset, int whence);
long ftell(FILE *stream);
FILE *freopen(const char *path, const char *mode, FILE *stream);
int fflush(FILE *stream);
int feof(FILE *stream);
int ferror(FILE *stream);
int ferror_unlocked(FILE *stream);
void clearerr(FILE *stream);
int fileno_unlocked(FILE *stream);
int fileno(FILE *stream);
int remove(const char *path);
void rewind(FILE *stream);
FILE *tmpfile(void);
int fgetpos(FILE *stream, fpos_t *pos);
int fsetpos(FILE *stream, const fpos_t *pos);
void setbuf(FILE *stream, char *buf);
int setvbuf(FILE *stream, char *buf, int mode, size_t size);
void perror(const char *s);
ssize_t getline(char **lineptr, size_t *n, FILE *stream);
int sscanf(const char *str, const char *fmt, ...);

int rename(const char *oldpath, const char *newpath);
