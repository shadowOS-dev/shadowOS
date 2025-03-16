#ifndef PROC_USER_H
#define PROC_USER_H

typedef struct
{
    char username[256];
    int uid;
    int gid;
    char description[256];
    char home_directory[256];
    char shell[256];
} user_t;

int parse_passwd(const char *input, user_t **users);
void users_init(const char *path);
char *get_username_by_uid(int uid);
user_t *get_user_by_uid(int uid);

#endif // PROC_USER_H