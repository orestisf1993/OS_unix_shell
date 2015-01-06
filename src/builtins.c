#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define BUILTINS_NUM 3
#define UNUSED(x) (void)(x)

enum {
    EXIT_CALL = 0,
    CD_CALL,
    JOBS_CALL
} builtin_names;

const char *builtins[BUILTINS_NUM];
//~ void (*handle_builtins[BUILTINS_NUM]) (void *_p);

void shell_exit()
{
    /* kill all background processes first and then exit shell */
    exit(0);
}

void change_directory(char *dir)
{
    UNUSED(dir);
}

void jobs()
{
}

void init()
{
    /* initialize builtins array */

    builtins[EXIT_CALL] = "exit";
    builtins[CD_CALL] = "cd";
    builtins[JOBS_CALL] = "jobs";
}


int check_builtins(char *cmd)
{
    int i;
    for (i = 0; i < BUILTINS_NUM; ++i) {
        if (strcmp(cmd, builtins[i]) == 0) return i;
    }
    return -1;
}
