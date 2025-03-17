# shadowOS Syscall List
# Syscall Table for shadowOS

| Syscall ID | Name        | Arguments                                      | Description                                                                                                              |
|------------|-------------|------------------------------------------------|--------------------------------------------------------------------------------------------------------------------------|
| 0          | `exit`  | `int code`                                         | Terminates the current process with the given exit code.                                                                 |
| 1          | `open`  | `const char *path`                                 | Opens a file or directory by the specified path. Returns a file descriptor on success, or `-1` on error.                 |
| 2          | `close` | `int fd`                                           | Closes the file descriptor `fd`. Returns `0` on success, `-1` on error.                                                  |
| 3          | `write` | `int fd`, `void *buff`, `size_t size`              | Writes `size` bytes from `buff` to the file described by `fd`. Returns the number of bytes written or `-1` on error.     |
| 4          | `read`  | `int fd`, `void *buff`, `size_t size`              | Reads up to `size` bytes from the file described by `fd` into `buff`. Returns the number of bytes read or `-1` on error. |
| 5          | `stat`  | `int fd`, `stat_t *stat`                           | Retrieves the status of the file descriptor `fd` and stores it in `stat`. Returns `0` on success, `-1` on error.         |
