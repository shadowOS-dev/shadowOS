#ifndef PROC_DATA_ELF_H
#define PROC_DATA_ELF_H

#include <stdint.h>

uint64_t elf_load_binary(void *data, uint64_t *pagemap);

#endif // PROC_DATA_ELF_H