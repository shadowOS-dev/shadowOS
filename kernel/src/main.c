#include <limine.h>
#include <util/cpu.h>
#include <stddef.h>
#include <stdbool.h>
#include <dev/portio.h>
#include <lib/printf.h>
#include <lib/log.h>
#include <sys/gdt.h>
#include <sys/intr.h>
#include <mm/pmm.h>
#include <mm/vmm.h>
#include <mm/vma.h>
#include <mm/kmalloc.h>
#include <lib/memory.h>
#include <lib/assert.h>
#include <lib/flanterm/backends/fb.h>
#include <lib/flanterm/flanterm.h>
#include <dev/vfs.h>
#include <fs/ramfs.h>
#include <sys/pci.h>
#include <sys/pic.h>
#include <fs/devfs.h>
#include <fs/procfs.h>
#include <dev/timer/pit.h>

struct limine_framebuffer *framebuffer = NULL;
uint64_t hhdm_offset = 0;
uint64_t __kernel_phys_base;
uint64_t __kernel_virt_base;
vma_context_t *kernel_vma_context = NULL;
uint64_t kernel_stack_top = 0;

struct flanterm_context *ft_ctx_priv = NULL;
struct flanterm_context *ft_ctx = NULL;

void (*putchar_impl)(char);
vnode_t *stdout = NULL;

extern char printk_buff[];
extern size_t printk_index;

__attribute__((used, section(".limine_requests"))) static volatile LIMINE_BASE_REVISION(3);
__attribute__((used, section(".limine_requests"))) static volatile struct limine_framebuffer_request framebuffer_request = {
    .id = LIMINE_FRAMEBUFFER_REQUEST,
    .revision = 0};
__attribute__((used, section(".limine_requests"))) static volatile struct limine_memmap_request memmap_request = {
    .id = LIMINE_MEMMAP_REQUEST,
    .response = 0};
__attribute__((used, section(".limine_requests"))) static volatile struct limine_hhdm_request hhdm_request = {
    .id = LIMINE_HHDM_REQUEST,
    .response = 0};
__attribute__((used, section(".limine_requests"))) static volatile struct limine_executable_address_request kernel_address_request = {
    .id = LIMINE_EXECUTABLE_ADDRESS_REQUEST,
    .response = 0};
__attribute__((used, section(".limine_requests"))) static volatile struct limine_module_request module_request = {
    .id = LIMINE_MODULE_REQUEST,
    .response = 0};
__attribute__((used, section(".limine_requests_start"))) static volatile LIMINE_REQUESTS_START_MARKER;
__attribute__((used, section(".limine_requests_end"))) static volatile LIMINE_REQUESTS_END_MARKER;

void putchar(char c)
{
    flanterm_write(ft_ctx_priv, &c, 1);
}

#define isprint(c) ((c) >= 32 && (c) <= 126)

void hex_dump_region(const void *data, size_t start_addr, size_t end_addr)
{
    const unsigned char *byte = (const unsigned char *)data;

    for (size_t i = start_addr; i < end_addr + 1; i++)
    {
        if (i % 16 == 0)
        {
            if (i != start_addr)
                printf("|");

            printf("\n%08zx  ", i);
        }

        // Print hex bytes
        printf("%02x ", byte[i]);

        if ((i + 1) % 8 == 0)
            printf(" ");

        if (i % 16 == 15 || i == end_addr)
        {
            printf("|");

            for (size_t j = i - (i % 16); j <= i; j++)
            {
                printf("%c", (isprint(byte[j]) ? byte[j] : '.'));
            }
        }
    }
    printf("|\n");
}

void kmain(void)
{
    // Save the kernel stack top, given via RSP
    __asm__ volatile("movq %%rsp, %0" : "=r"(kernel_stack_top));

    if (LIMINE_BASE_REVISION_SUPPORTED == false)
    {
        error("Unsupported LIMINE base revision, halting");
        hcf();
    }

    if (framebuffer_request.response == NULL || framebuffer_request.response->framebuffer_count < 1)
    {
        error("No framebuffer available, halting");
        hcf();
    }

    kprintf("shadowOS Kernel v1.0 (c) Copyright 2025 Kevin Alavik <kevin@alavik.se>\n");

    framebuffer = framebuffer_request.response->framebuffers[0];

    ft_ctx_priv = flanterm_fb_init(
        NULL,
        NULL,
        framebuffer->address, framebuffer->width, framebuffer->height, framebuffer->pitch,
        framebuffer->red_mask_size, framebuffer->red_mask_shift,
        framebuffer->green_mask_size, framebuffer->green_mask_shift,
        framebuffer->blue_mask_size, framebuffer->blue_mask_shift,
        NULL,
        NULL, NULL,
        NULL, NULL,
        NULL, NULL,
        NULL, 0, 0, 1,
        0, 0,
        0);
    assert(ft_ctx_priv != NULL);
    ft_ctx_priv->cursor_enabled = false;
    ft_ctx_priv->full_refresh(ft_ctx_priv);
    ft_ctx = NULL;
    // Enable the flanterm console
    ft_ctx = ft_ctx_priv;

    gdt_init();
    idt_init();
    load_idt();

    __asm__ volatile("cli");
    pic_init();
    __asm__ volatile("sti");

    if (hhdm_request.response == NULL)
    {
        error("No HHDM available, halting");
        hcf();
    }

    hhdm_offset = hhdm_request.response->offset;

    pmm_init(memmap_request.response);
    if (kernel_address_request.response == NULL)
    {
        error("No kernel address available, halting");
        hcf();
    }

    __kernel_phys_base = kernel_address_request.response->physical_base;
    __kernel_virt_base = kernel_address_request.response->virtual_base;

    vmm_init();
    // pmm_vmm_cleanup(memmap_request.response);
    kernel_vma_context = vma_create_context(kernel_pagemap);
    if (kernel_vma_context == NULL || kernel_vma_context->root == NULL)
    {
        error("Failed to create kernel VMA context, halting");
        hcf();
    }

    vfs_init();

    msg_assert(module_request.response, "No modules passed to the kernel, expected at least one");

    // unsafe af, but works for now
    uint8_t *ramfs_data = (uint8_t *)module_request.response->modules[0]->address;
    size_t ramfs_size = module_request.response->modules[0]->size;

    // Mount on / and change type to ramfs
    root_mount->type = strdup("ramfs");
    assert(root_mount);
    ramfs_init(root_mount, RAMFS_TYPE_USTAR, ramfs_data, ramfs_size);

    // Setup devfs and procfs
    devfs_init();
    procfs_init();

    // pci shit
    pci_debug_log();

    // Setup the procfs nodes
    BLOCK_START("procfs_setup")
    {
        // Setup /proc/uptime
        assert(procfs_add_proc("uptime", "0.00 0.00", 9) == 0);

        // Setup /proc/cpuinfo
        uint32_t eax, ebx, ecx, edx;
        char vendor[13];
        eax = 0;
        cpuid(eax, &ebx, &ecx, &edx);
        *(uint32_t *)vendor = ebx;
        *(uint32_t *)(vendor + 4) = edx;
        *(uint32_t *)(vendor + 8) = ecx;
        vendor[12] = '\0';

        char cpuinfo[512];
        int len = snprintf(cpuinfo, sizeof(cpuinfo), "vendor_id: %s\n", vendor);
        assert((uint64_t)len < sizeof(cpuinfo));
        assert(procfs_add_proc("cpuinfo", cpuinfo, len) == 0);
    }
    BLOCK_END("procfs_setup")

    // Setup boot    log
    BLOCK_START("boot_log")
    {
        // NOTE: the /var/log/boot.log file should already exist in the initramfs, if not tough luck lmao.
        // write the printk buffer to the /var/log/boot.log file
        vnode_t *log = vfs_lazy_lookup(VFS_ROOT()->mount, "/var/log/boot.log");
        fwrite(log, &printk_buff_start, printk_index);
        vfs_write(log, "\0", 1, log->size);
        memset(&printk_buff_start, 0, printk_index);
        printk_index = 0;
    }
    BLOCK_END("boot_log")

    // clear screen becuz we are done
    ft_ctx->clear(ft_ctx, true);

    // Disable writing directly to the flanterm context, since kprintf will be disabled anyways.
    ft_ctx = NULL;

    BLOCK_START("stdout_setup")
    {
        // You cant read directly from stdout
        void read(void *buf, size_t size, size_t offset)
        {
            (void)buf;
            (void)size;
            (void)offset;
            return;
        }

        void write(const void *buf, size_t size, size_t offset)
        {
            (void)offset;
            assert(buf);
            for (size_t i = 0; i < size; i++)
            {
                putchar(*(char *)((uint8_t *)buf + i));
                // output to serial also for now
                outb(0xE9, *(char *)((uint8_t *)buf + i));
            }
        }
        devfs_add_dev("stdout", read, write);
    }
    BLOCK_END("stdout_setup")

    // Setup /dev/stdout
    stdout = vfs_lazy_lookup(VFS_ROOT()->mount, "/dev/stdout");
    assert(stdout);

    // initialize timer and scheduler
    pit_init();

    // Print the first 70 bytes of the boot log
    vnode_t *log = vfs_lazy_lookup(VFS_ROOT()->mount, "/var/log/boot.log");
    assert(log);
    char *buf = kmalloc(log->size);
    vfs_read(log, buf, log->size, 0);
    assert(buf);
    fwrite(stdout, buf, 70);
    kfree(buf);
    printf("\n");

    // we done
    printf("uptime: %s\n", VFS_READ("/proc/uptime"));
    printf("%s", VFS_READ("/proc/cpuinfo"));
    printf("\n");

    // print the root filesystem
    vnode_t *current = VFS_ROOT()->child;
    vnode_t *stack[256];
    int stack_depth = 0;

    while (current != NULL || stack_depth > 0)
    {
        if (current != NULL)
        {
            VFS_PRINT_VNODE(current);
            if (current->child != NULL)
            {
                stack[stack_depth++] = current->next;
                current = current->child;
            }
            else
            {
                current = current->next;
            }
        }
        else
        {
            current = stack[--stack_depth];
        }
    }

    printf("\n");

    // print out free memory
    uint64_t free = pmm_get_free_memory();
    uint64_t total = pmm_get_total_memory();
    printf("Free memory: %llu MB\nTotal memory: %llu MB\n", BYTES_TO_MB(free), BYTES_TO_MB(total));
    hlt();
}
