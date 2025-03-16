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

    char *buffer = kmalloc(strlen(input) + 1);
    assert(buffer);
    strncpy(buffer, input, strlen(input) + 1);

    char *line;
    char *saveptr;
    int count = 0;

    user_t *temp_users = (user_t *)kmalloc(100 * sizeof(user_t));
    assert(temp_users);

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
            strncpy(temp_users[count].username, token, sizeof(temp_users[count].username) - 1);
            temp_users[count].username[sizeof(temp_users[count].username) - 1] = '\0';
        }

        token = strtok(NULL, ":");
        token = strtok(NULL, ":");
        if (token != NULL)
            temp_users[count].uid = strtol(token, NULL, 10);

        token = strtok(NULL, ":");
        if (token != NULL)
            temp_users[count].gid = strtol(token, NULL, 10);

        token = strtok(NULL, ":");
        if (token != NULL)
        {
            strncpy(temp_users[count].description, token, sizeof(temp_users[count].description) - 1);
            temp_users[count].description[sizeof(temp_users[count].description) - 1] = '\0';
        }

        token = strtok(NULL, ":");
        if (token != NULL)
        {
            strncpy(temp_users[count].home_directory, token, sizeof(temp_users[count].home_directory) - 1);
            temp_users[count].home_directory[sizeof(temp_users[count].home_directory) - 1] = '\0';
        }

        token = strtok(NULL, ":");
        if (token != NULL)
        {
            strncpy(temp_users[count].shell, token, sizeof(temp_users[count].shell) - 1);
            temp_users[count].shell[sizeof(temp_users[count].shell) - 1] = '\0';
        }

        trace("Parsed user: %s uid=%d gid=%d", temp_users[count].username, temp_users[count].uid, temp_users[count].gid);
        count++;
        line = strtok_r(NULL, "\n", &saveptr);
    }

    *users = kmalloc(count * sizeof(user_t));
    if (*users == NULL)
    {
        kfree(temp_users);
        kfree(buffer);
        return -1;
    }

    memcpy(*users, temp_users, count * sizeof(user_t));

    kfree(temp_users);
    kfree(buffer);
    return count;
}

void users_init(const char *path)
{
    if (cached_users != NULL)
        return;

    char *passwd = VFS_READ(path);
    assert(passwd);

    num_cached_users = parse_passwd(passwd, &cached_users);
    if (num_cached_users < 0)
    {
        kfree(passwd);
        return;
    }

    for (int i = 0; i < num_cached_users; i++)
    {
        trace("Loaded user: %s uid=%d gid=%d", cached_users[i].username, cached_users[i].uid, cached_users[i].gid);
    }

    kfree(passwd);
}

char *get_username_by_uid(int uid)
{
    if (cached_users == NULL)
        users_init("/etc/passwd");

    for (int i = 0; i < num_cached_users; i++)
    {
        if (cached_users[i].uid == uid)
            return cached_users[i].username;
    }

    warning("Did not find user with uid: %d", uid);
    return "unknown";
}

user_t *get_user_by_uid(int uid)
{
    if (cached_users == NULL)
        users_init("/etc/passwd");

    for (int i = 0; i < num_cached_users; i++)
    {
        if (cached_users[i].uid == uid)
            return &cached_users[i];
    }

    warning("Did not find user with uid: %d", uid);
    return NULL;
}
