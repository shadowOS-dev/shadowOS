#ifndef MM_VMM_H
#define MM_VMM_H

#include <stdint.h>

#define VMM_PRESENT 0x01
#define VMM_WRITE 0x02
#define VMM_NX (1ull << 63)
#define VMM_USER 0x04

extern uint64_t *kernel_pagemap;

void vmm_init();
void vmm_switch_pagemap(uint64_t *pagemap);
uint64_t *vmm_new_pagemap();
void vmm_map(uint64_t *pagemap, uint64_t virt, uint64_t phys, uint64_t flags);
void vmm_unmap(uint64_t *pagemap, uint64_t virt);
uint64_t virt_to_phys(uint64_t *pagemap, uint64_t virt);
void vmm_destroy_pagemap(uint64_t *pagemap);

#endif // MM_VMM_H