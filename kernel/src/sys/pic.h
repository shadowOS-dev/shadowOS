#ifndef SYS_PIC_H
#define SYS_PIC_H

#include <stdint.h>
#include <stdbool.h>

#define PIC_REMAP_OFFSET 0x20

void pic_setmask(uint16_t new_mask);
uint16_t pic_getmask();
void pic_configure(uint8_t offset_pic1, uint8_t offset_pic2, bool auto_eoi);
void pic_sendendofinterrupt(int irq);
void pic_disable();
void pic_enable();
void pic_mask(int irq);
void pic_unmask(int irq);
uint16_t pic_readirqrequestregister();
uint16_t pic_readinserviceregister();

#endif // SYS_PIC_H