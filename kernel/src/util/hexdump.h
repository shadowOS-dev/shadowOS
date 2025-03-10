#ifndef UTIL_HEXDUMP_H
#define UTIL_HEXDUMP_H

#include <stdint.h>
#include <stddef.h>

#define _hd_isprint(c) ((c) >= 32 && (c) <= 126)
#define ROUND_UP_TO_16(x) (((x) + 15) & ~15)

static void hex_dump_region(int (*printf)(const char *, ...), const void *data, size_t start_addr, size_t end_addr)
{
    const unsigned char *byte = (const unsigned char *)data;
    end_addr = ROUND_UP_TO_16(end_addr);

    for (size_t i = start_addr; i < end_addr; i++)
    {
        if (i % 16 == 0)
        {
            if (i != start_addr)
                printf("  |  ");
            printf("\n%08zx  ", i);
        }

        printf("%02x ", byte[i]);

        if ((i + 1) % 8 == 0)
            printf(" ");

        if (i % 16 == 15 || i == end_addr - 1)
        {
            printf("  | ");

            for (size_t j = i - (i % 16); j <= i; j++)
            {
                printf("%c", (_hd_isprint(byte[j]) ? byte[j] : '.'));
            }
        }
    }

    printf("  | \n");
}

#endif // UTIL_HEXDUMP_H
