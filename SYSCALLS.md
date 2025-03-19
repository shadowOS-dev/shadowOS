# shadowOS Syscall List
| Syscall ID | Name    | Arguments                                      | Description                                                                                                               |
|------------|---------|------------------------------------------------|---------------------------------------------------------------------------------------------------------------------------|
| 0          | `exit`  | `int code`                                     | Terminates the current process with the given exit code. This is typically used to indicate the completion or failure of a process. The exit code is returned to the parent process or the operating system. |
| 1          | `open`  | `const char *path`, `uint64_t flags`, `uint8_t kind` | Opens a file or directory at the specified `path`. The `flags` argument specifies how the file should be opened, and the `kind` argument specifies the type of file (e.g., regular file, directory).  |
|            |         |                                                | **Flags:**                                                                                                                |
|            |         |                                                | - `O_CREATE`: Create the file if it doesn't exist.                                                                         |
|            |         |                                                | **Kind:**                                                                                                                |
|            |         |                                                | - `0x0001`: Directory                                                                                                       |
|            |         |                                                | - `0x0002`: Regular file                                                                                                          |
| 2          | `close` | `int fd`                                       | Closes the file descriptor `fd`, releasing the resources associated with it. Returns `0` on success and `-1` on error.     |
| 3          | `write` | `int fd`, `void *buff`, `size_t size`          | Writes `size` bytes from the buffer `buff` to the file or device described by `fd`. Returns the number of bytes written or `-1` on error. |
| 4          | `read`  | `int fd`, `void *buff`, `size_t size`          | Reads up to `size` bytes from the file or device described by `fd` into the buffer `buff`. Returns the number of bytes read or `-1` on error. |
| 5          | `stat`  | `int fd`, `stat_t *stat`                       | Retrieves the status information of the file or device associated with `fd`. The `stat` struct is populated with the file's metadata, such as its size, permissions, and modification time. Returns `0` on success or `-1` on error. |

---

### Detailed Description of Syscalls

#### 1. `exit`
- **Arguments:**
  - `code`: The exit code, an integer that indicates how the process terminated. Common exit codes are `0` for success and non-zero for errors.
- **Behavior:** Terminates the current process. The exit code is passed back to the parent process or operating system.

#### 2. `open`
- **Arguments:**
  - `path`: The path to the file or directory to open.
  - `flags`: Specifies how to open the file. The `O_CREATE` flag can be used to create the file if it does not exist.
  - `kind`: Specifies the type of object being opened, such as a regular file (0) or directory (1).
- **Flags:**
  - `O_CREATE`: Create the file if it doesn't already exist. This flag ensures the file is created if it is missing.
- **Behavior:** Opens a file or directory for reading, writing, or both based on the flags.

#### 3. `close`
- **Arguments:**
  - `fd`: The file descriptor to close.
- **Behavior:** Closes the specified file descriptor and frees the resources associated with it. Once closed, the file descriptor cannot be used for further I/O operations.

#### 4. `write`
- **Arguments:**
  - `fd`: The file descriptor to write to (e.g., a file or standard output).
  - `buff`: A pointer to the buffer that contains the data to be written.
  - `size`: The number of bytes to write from the buffer to the file.
- **Behavior:** Writes data from the buffer to the file or device associated with `fd`. The number of bytes written is returned, or `-1` is returned on error.

#### 5. `read`
- **Arguments:**
  - `fd`: The file descriptor to read from (e.g., a file or standard input).
  - `buff`: A pointer to the buffer where the read data will be stored.
  - `size`: The number of bytes to read into the buffer.
- **Behavior:** Reads data from the file or device described by `fd` into the provided buffer. The number of bytes read is returned, or `-1` on error.

#### 6. `stat`
- **Arguments:**
  - `fd`: The file descriptor of the file or device whose status is being retrieved.
  - `stat`: A pointer to a `stat_t` structure where the file's metadata will be stored.
- **Behavior:** Retrieves the status of the file or device associated with `fd` and stores the information in the `stat_t` structure. This typically includes file size, permissions, and modification times.

---

### Notes:
- The syscall interface provides low-level operations for interacting with the operating system's kernel and the file system. Each syscall handles a different aspect of process and file management, making it possible to perform basic I/O operations, file management, and process control.
