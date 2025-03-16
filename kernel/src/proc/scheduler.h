#ifndef PROC_SCHEDULER_H
#define PROC_SCHEDULER_H

#include <stdint.h>
#include <sys/intr.h>
#include <dev/vfs.h>
#include <proc/errno.h>

#define PROC_DEFAULT_TIME 1 // Roughly 20ms, timer is expected to run at roughly 200hz
#define PROC_MAX_PROCS 2048 // that should be plenty
#define PROC_MAX_FDS 256    // that shuold hopefully be plenty

typedef enum
{
    PROCESS_READY,
    PROCESS_RUNNING,
    PROCESS_WAITING,
    PROCESS_TERMINATED
} process_state_t;

// Process control block, some information
typedef struct pcb
{
    struct register_ctx ctx;
    uint64_t pid;
    process_state_t state;
    uint64_t timeslice;
    uint64_t *pagemap;
    vnode_t **fd_table;
    uint64_t fd_count;
    errno_t errno;
    uint32_t whoami; // Current user
} pcb_t;

void scheduler_init();
uint64_t scheduler_spawn(void (*entry)(void), uint64_t *pagemap);
void scheduler_tick(struct register_ctx *ctx);
void scheduler_exit(int return_code);
pcb_t *scheduler_get_current();
int scheduler_proc_add_vnode(uint64_t pid, vnode_t *node);
int scheduler_proc_remove_vnode(uint64_t pid, int fd);
int scheduler_proc_change_whoami(uint64_t pid, int uid);

#endif // PROC_SCHEDULER_H