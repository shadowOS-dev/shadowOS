#ifndef PROC_SCHEDULER_H
#define PROC_SCHEDULER_H

#include <stdint.h>
#include <sys/intr.h>

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
} pcb_t;

#define PROC_DEFAULT_TIME 1 // Roughly 20ms, timer is expected to run at roughly 200hz
#define PROC_MAX_PROCS 2048 // that should be plenty

void scheduler_init();
uint64_t scheduler_spawn(void (*entry)(void), uint64_t *pagemap);
void scheduler_tick(struct register_ctx *ctx);
void scheduler_terminate(uint64_t pid);
void scheduler_exit(int return_code);
pcb_t *scheduler_get_current();

#endif // PROC_SCHEDULER_H