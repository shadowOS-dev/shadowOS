#ifndef DEV_STDOUT_H
#define DEV_STDOUT_H

#include <dev/vfs.h>

extern vnode_t *stdout;

void stdout_init();

#endif // DEV_STDOUT_H