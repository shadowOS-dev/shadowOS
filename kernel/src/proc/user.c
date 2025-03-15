#include <proc/user.h>
#include <lib/memory.h>
#include <stddef.h>
#include <mm/kmalloc.h>
#include <lib/assert.h>
#include <dev/vfs.h>

static user_t *cached_users = NULL;
static int num_cached_users = 0;

int parse_passwd(const char *input, user_t **users)
{
    assert(input);
    char *line;
    char *buffer = kmalloc(strlen(input) + 1);
    assert(buffer);
    char *saveptr;
    int count = 0;

    user_t *temp_users = (user_t *)kmalloc(100 * sizeof(user_t));
    assert(temp_users);

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
            strncpy(temp_users[count].username, token, sizeof(temp_users[count].username));
        }

        token = strtok(NULL, ":");
        if (token != NULL)
        {
            temp_users[count].uid = strtol(token, NULL, 10);
        }

        token = strtok(NULL, ":");
        if (token != NULL)
        {
            temp_users[count].gid = strtol(token, NULL, 10);
        }

        token = strtok(NULL, ":");
        if (token != NULL)
        {
            strncpy(temp_users[count].description, token, sizeof(temp_users[count].description));
        }

        token = strtok(NULL, ":");
        if (token != NULL)
        {
            strncpy(temp_users[count].home_directory, token, sizeof(temp_users[count].home_directory));
        }

        token = strtok(NULL, ":");
        if (token != NULL)
        {
            strncpy(temp_users[count].shell, token, sizeof(temp_users[count].shell));
        }

        count++;
        line = strtok_r(NULL, "\n", &saveptr);
    }

    *users = kmalloc(count * sizeof(user_t));
    memcpy(*users, temp_users, count * sizeof(user_t));

    kfree(temp_users);
    kfree(buffer);
    return count;
}

void users_init(const char *path)
{
    if (cached_users != NULL)
    {
        return;
    }

    char *passwd = VFS_READ(path);
    assert(passwd);

    num_cached_users = parse_passwd(passwd, &cached_users);

    kfree(passwd);
}

char *get_username_by_uid(int target_uid)
{
    if (cached_users == NULL)
    {
        users_init("/etc/passwd");
    }

    for (int i = 0; i < num_cached_users; i++)
    {
        if (cached_users[i].uid == target_uid)
        {
            return cached_users[i].username;
        }
    }

    return NULL;
}

user_t *get_user_by_uid(int uid)
{

    for (int i = 0; i < num_cached_users; i++)
    {
        if (cached_users[i].uid == uid)
        {
            return &cached_users[i];
        }
    }

    return NULL;
}