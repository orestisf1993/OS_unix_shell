#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <readline/history.h>
#include <errno.h>

#include "utils.h"

const builtins_struct builtins[BUILTINS_NUM] = {
    {EXIT_CMD  , "exit" , shell_exit          , "usage:\nexit [exit_code]\n"},
    {CD_CMD    , "cd"   , change_directory    , "usage:\ncd [dir]\n\nChange current working directory to [dir] directory\nif [dir] is blank, change the directory to HOME environmental variable\n"},
    {JOBS_CMD  , "jobs" , jobs_list           , "usage:\njobs\n\nlist all active processes.\n"},
    {HELP_CMD  , "help" , print_help          , "usage:\nhelp [cmd]\n\nShow help for command [cmd].\nIf [cmd] is blank show this text.\n"}
};

#define UNUSED(x) (void)(x)
#define PRINT_NO_ARGS_MSG(thing_name) {printf("%s doesn't take that many arguments\n", thing_name);}

void print_help(int argc, char** argv)
{
    int code;
    if (argc == 2) {
        code = check_if_builtin(argv[1]);
    } else code = HELP_CMD;

    if (code == -1) fprintf(stderr, "command %s not found!\n", argv[1]);
    else {
        printf("Showing help for command: %s\n", builtins[code].cmd);
        printf("-------------------------\n");
        printf("%s\n", builtins[code].help_text);
    }
    if (code == HELP_CMD){
        int i;
        printf("All available commands:\n");
        for (i=0; i<BUILTINS_NUM; ++i) printf("%s\n", builtins[i].cmd);
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


void jobs_list(int argc, char** argv)
{
    process *p;

    if (argc > 1) PRINT_NO_ARGS_MSG(argv[0]);

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

void change_directory(int argc, char **argv )
{
    if(argc > 2) {
        PRINT_NO_ARGS_MSG(argv[0]);
    } else if (argc == 1) {
        /* no arguments after 'cd', change directory to HOME */
        chdir(getenv("HOME"));
        /* change directory to argv[1] */
    } else if( (chdir(argv[1]) ) == -1 ) {
        /* error in chdir */
        perror(argv[0]);
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
