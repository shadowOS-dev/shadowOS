void module_init(void)
{
    __asm__ volatile("int $0x01"); // throw debug interrupt to test loading.
}