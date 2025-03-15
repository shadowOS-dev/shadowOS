#ifndef PROC_ERRNO_H
#define PROC_ERRNO_H

typedef unsigned int errno_t;

#define EOK 0      // No error
#define ENOENT 1   // No such file or directory
#define EBADF 2    // Bad file descriptor
#define EFAULT 3   // Bad address
#define EINVAL 4   // Invalid argument
#define EBADPID 5  // Bad pid
#define ENOTIMPL 6 // Function not implemented

#define ERRNO_TO_STR(errno)                                                        \
    ((errno) == EOK ? "No error" : (errno) == ENOENT ? "No such file or directory" \
                               : (errno) == EBADF    ? "Bad file descriptor"       \
                               : (errno) == EFAULT   ? "Bad address"               \
                               : (errno) == EINVAL   ? "Invalid argument"          \
                               : (errno) == EBADPID  ? "Bad pid"                   \
                               : (errno) == ENOTIMPL ? "Function not implemented"  \
                                                     : "Unknown error")

#endif // PROC_ERRNO_H
