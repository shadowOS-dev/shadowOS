#include <proc/group.h>
#include <lib/memory.h>
#include <stddef.h>
#include <mm/kmalloc.h>
#include <lib/assert.h>
#include <dev/vfs.h>

static group_t *cached_groups = NULL;
static int num_cached_groups = 0;

int parse_group(const char *input, group_t **groups)
{
    assert(input);
    char *line;
    char *buffer = kmalloc(strlen(input) + 1);
    assert(buffer);
    char *saveptr;
    int count = 0;

    group_t *temp_groups = (group_t *)kmalloc(100 * sizeof(group_t));
    assert(temp_groups);

    strncpy(buffer, input, strlen(input) + 1);
    line = strtok_r(buffer, "\n", &saveptr);
    while (line != NULL)
    {
        if (line[0] == '\0' || line[0] == '#')
        {
            line = strtok_r(NULL, "\n", &saveptr);
            continue;
        }

        char *token;
        token = strtok(line, ":");
        if (token != NULL)
        {
            strncpy(temp_groups[count].groupname, token, sizeof(temp_groups[count].groupname));
        }

        token = strtok(NULL, ":");
        if (token != NULL)
        {
            temp_groups[count].gid = strtol(token, NULL, 10);
        }

        token = strtok(NULL, ":");
        if (token != NULL)
        {
            strncpy(temp_groups[count].members, token, sizeof(temp_groups[count].members));
        }

        count++;
        line = strtok_r(NULL, "\n", &saveptr);
    }

    *groups = kmalloc(count * sizeof(group_t));
    memcpy(*groups, temp_groups, count * sizeof(group_t));

    kfree(temp_groups);
    kfree(buffer);
    return count;
}

void groups_init(const char *path)
{
    if (cached_groups != NULL)
    {
        return;
    }

    char *group_file = VFS_READ(path);
    assert(group_file);

    num_cached_groups = parse_group(group_file, &cached_groups);

    kfree(group_file);
}

char *get_groupname_by_gid(int target_gid)
{
    if (cached_groups == NULL)
    {
        groups_init("/etc/group");
    }

    for (int i = 0; i < num_cached_groups; i++)
    {
        if (cached_groups[i].gid == target_gid)
        {
            return cached_groups[i].groupname;
        }
    }

    return "unknown";
}
