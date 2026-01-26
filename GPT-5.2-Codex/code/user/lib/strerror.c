#include <string.h>
#include <errno.h>

const char *strerror(int err) {
  switch (err) {
    case EPERM:
      return "Operation not permitted";
    case ENOENT:
      return "No such file or directory";
    case EIO:
      return "I/O error";
    case EBADF:
      return "Bad file descriptor";
    case EACCES:
      return "Permission denied";
    case EEXIST:
      return "File exists";
    case ENOTDIR:
      return "Not a directory";
    case EISDIR:
      return "Is a directory";
    case EINVAL:
      return "Invalid argument";
    case ENOSYS:
      return "Function not implemented";
    case ENOTEMPTY:
      return "Directory not empty";
    case ENOMEM:
      return "Out of memory";
    case ENOSPC:
      return "No space left on device";
    default:
      return "Unknown error";
  }
}
