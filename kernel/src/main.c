#include <limine.h>
#include <util/cpu.h>
#include <stddef.h>
#include <stdbool.h>
#include <dev/portio.h>
#include <lib/printf.h>
#include <lib/log.h>
#include <sys/gdt.h>
#include <sys/idt.h>
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
vnode_t *tty = NULL;

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

#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define MAX(a, b) ((a) > (b) ? (a) : (b))

void putchar(char c)
{
    flanterm_write(ft_ctx_priv, &c, 1);
}

void kmain(void)
{
    // Save the kernel stack top, given via RSP
    __asm__ volatile("movq %%rsp, %0" : "=r"(kernel_stack_top));

    printf("\033c");
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

    // timer shit and scheduler
    trace("pic can suck my ass since it doesnt wanna fucking work, die");

    // clear screen becuz we are done
    ft_ctx->clear(ft_ctx, true);

    // Disable writing directly to the flanterm context, and setup TTY putchar callback
    ft_ctx = NULL;

    BLOCK_START("tty0_setup")
    {
        // You cant read directly from a tty
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
            }
        }
        devfs_add_dev("tty0", read, write);
    }
    BLOCK_END("tty0_setup")

    // Setup the procfs nodes
    BLOCK_START("procfs_setup")
    {
        assert(procfs_add_proc("uptime", "0.00 0.00", 9) == 0);
    }

    BLOCK_END("procfs_setup")

    // Read the welcome text
    vnode_t *w = vfs_lazy_lookup(root_mount, "/root/welcome.txt");
    assert(w);
    char *buf = kmalloc(w->size + 1);
    buf[w->size] = 0;
    vfs_read(w, buf, w->size, 0);
    debug("%s", buf);
    kfree(buf);

    // Setup /dev/tty0
    tty = vfs_lazy_lookup(root_mount, "/dev/tty0");
    assert(tty);
    TTY_WRITE(tty, "Welcome to shadowOS\n");

    // re-enable the flanterm context for direct printf support.
    ft_ctx = ft_ctx_priv;

    // we done
    char *uptime = VFS_READ("/proc/uptime");
    printf("uptime: %s\n", uptime);
    printf("\n");

    // Print out the root tree
    BLOCK_START("vfs_root_print")
    {
        // printf("Flag   | Type | Size | Path         \n");
        // printf("-------|------|------|--------------\n");
        vfs_debug_print(VFS_ROOT()->mount);
    }
    BLOCK_END("vfs_root_print")

    printf("\n");

    // initialize timer and other time shit
    pit_init();

    hlt();
}
