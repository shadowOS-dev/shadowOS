#ifndef PROC_GROUP_H
#define PROC_GROUP_H

typedef struct
{
    char groupname[256];
    int gid;
    char members[1024]; // List of comma-separated user names
} group_t;

int parse_group(const char *input, group_t **groups);
void groups_init(const char *path);
char *get_groupname_by_gid(int target_gid);

#endif // PROC_GROUP_H
