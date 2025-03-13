void _start(void)
{
    __asm__ volatile("int $0x01");
    while (1)
        __asm__ volatile("hlt");
}
