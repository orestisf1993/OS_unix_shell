#define BUILTINS_NUM 3
const char *builtins[BUILTINS_NUM];
const void *handle_builtins[BUILTINS_NUM];

void init()
{
    /* initialize builtins array */
    builtins[0] = "exit";
    builtins[1] = "cd";
    builtins[2] = "jobs";
}


int check_builtins(char *cmd)
{
    int i;
    for (i = 0; i < BUILTINS_NUM; ++i) {
        if (strcmp(cmd, builtins[i]) == 0) return i;
    }
    return -1;
}
