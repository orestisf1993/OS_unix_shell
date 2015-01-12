#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <readline/history.h>
#include <errno.h>

#include "utils.h"

builtins_struct builtins[BUILTINS_NUM] = {
    {EXIT_CMD  , "exit" , shell_exit          , "blank help text\n"},
    {CD_CMD    , "cd"   , change_directory    , "blank help text\n"},
    {JOBS_CMD  , "jobs" , list_all            , "blank help text\n"},
    {HELP_CMD  , "help" , print_help          , "blank help text\n"}
};

#define UNUSED(x) (void)(x)
#define PRINT_NO_ARGS_MSG(thing_name) printf("%s doesn't take any arguments\n", thing_name);

void print_help(int argc, char** argv)
{
    printf("HELP:\n");

    if (argc == 2) {
        int code = check_if_builtin(argv[1]);
        if (code == -1) fprintf(stderr, "command not found!\n");
        else printf("%s\n", builtins[code].help_text);
    } else {
        printf("generic help\n");
    }
}

void free_all()
{
    process *p;
    process *pr;
    pr = head;
    for (p = head->next; p != NULL; p = p->next) {
        free(pr);
        pr = p;
    }
}

void shell_exit(int argc, char** argv)
{
    if (argc > 1) PRINT_NO_ARGS_MSG(argv[0]);

    free_all();
    write_history(NULL);
    exit(0);
}

void change_directory(int argc, char** argv)
{
    if (argc > 1) PRINT_NO_ARGS_MSG(argv[0]);
}

void list_all(int argc, char** argv)
{
    process *p;

    if (argc > 1) PRINT_NO_ARGS_MSG(argv[0]);

    printf("pid completed status\n");
    for (p = head; p != NULL; p = p->next) {
        if (p->pid) {
            printf("[%d] %s",
                   p->pid,
                   (p->completed) ? "COMPLETED" : "RUNNING");
            if (p->completed) {
                printf(" status: %d\n", p->status);
            } else printf("\n");
        }
    }
}


void call_builtin(int code, int argc, char** argv)
{
    builtins[code].action(argc, argv);
}

int check_if_builtin(char *cmd)
{
    int code;
    for (code = 0; code < BUILTINS_NUM; ++code) {
        if (strcmp(builtins[code].cmd, cmd) == 0) return code;
    }
    return -1;
}
