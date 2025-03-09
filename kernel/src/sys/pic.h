#ifndef SYS_PIC_H
#define SYS_PIC_H

#include <stdint.h>
#include <stdbool.h>

void pic_init();
void pic_eoi(uint8_t irq);
void pic_mask(uint8_t irq);
void pic_unmask(uint8_t irq);
void pic_maskall();

#endif // SYS_PIC_H