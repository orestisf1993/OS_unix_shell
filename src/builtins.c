#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <readline/history.h>
#include <errno.h>

#include "utils.h"

const builtins_struct builtins[BUILTINS_NUM] = {
    {EXIT_CMD    , "exit"   , shell_exit          , "usage:\nexit [exit_code]\n\nDefault value of [exit_code] is 0\n"},
    {CD_CMD      , "cd"     , change_directory    , "usage:\ncd [dir]\n\nChange current working directory to [dir] directory (spaces don't need to be escaped)\nif [dir] is blank, change the directory to HOME environmental variable\n"},
    {JOBS_CMD    , "jobs"   , jobs_list           , "usage:\njobs\n\nlist all active processes.\n"},
    {HELP_CMD    , "help"   , print_help          , "usage:\nhelp [cmd]\n\nShow help for command [cmd].\nIf [cmd] is blank show this text.\n"},
    {HOFF_CMD    , "hoff"   , history_off         , "usage:\nhoff\n\nhoff disables the history log\n"},
    {HON_CMD     , "hon"    , history_on          , "usage:\nhon\n\nhon enables the history log\n"},
    {PDEAD_CMD   , "pdead"  , print_dead          , "usage:\npdead [on|off]\n\n enables/disables printing of foreground processes' status on their death.\n"},
    {PWD_CMD     , "pwd"    , print_wd            , "usage:\npwd\n\n prints the current working directory.\n"}
};

/**
 * @brief Prints an invalid usage message.
 * @param thing_name usually argv[0] or function name.
 * @returns nothing
 */
#define PRINT_BAD_ARGS_MSG(thing_name) {printf("%s: invalid usage\n", thing_name);}

void print_wd(int argc, char** argv){
    extern char* shell_get_cwd();
    char* cwd;
    if (argc > 1) {
        PRINT_BAD_ARGS_MSG(argv[0]);
        return;
    }
    cwd = shell_get_cwd();
    printf("%s\n", cwd);
    free(cwd);
    return;
}

/**
 * @brief enable/disable always printing child death.
 * @param argc argument count, should be 2.
 * @param argv argv[1] should contain "on" or "off" string.
 */
void print_dead(int argc, char** argv){
    extern int always_print_dead;
    if (argc != 2) {
        PRINT_BAD_ARGS_MSG(argv[0]);
        return;
    }
    if (strcmp(argv[1], "on") == 0)
    {
        printf("print dead processes: %s -> ENABLED\n", always_print_dead? "ENABLED" : "DISABLED");
        always_print_dead = 1;
    }
    else if (strcmp(argv[1], "off") == 0)
    {
        printf("print dead processes: %s -> DISABLED\n", always_print_dead? "ENABLED" : "DISABLED");
        always_print_dead = 0;
    }
    else fprintf(stderr, "%s: invalid option\n", argv[0]);
}

int save_history_to_file = 1;
/**
 * @brief enables history logging.
 * @param argc unused
 * @param argv unused
 */
void history_on(int argc, char** argv)
{
    if (argc > 1) {
        PRINT_BAD_ARGS_MSG(argv[0]);
        return;
    }
    printf("history: %s -> ENABLED\n", save_history_to_file? "ENABLED" : "DISABLED");
    save_history_to_file = 1;
}

/**
 * @brief disables history logging.
 * @param argc unused
 * @param argv unused
 */
void history_off(int argc, char** argv)
{
    if (argc > 1) {
        PRINT_BAD_ARGS_MSG(argv[0]);
        return;
    }
    printf("history: %s -> DISABLED\n", save_history_to_file? "ENABLED" : "DISABLED");
    save_history_to_file = 0;
}

/**
 * @brief used by the help command. Print help string for a builtin command.
 * @param argc is 1 for generic help, is 2 for specific command help.
 * @param argv empty or contains the name of the command to print help for.
 *
 * print_help() searches the builtins global array for the given command
 * string. If not found, it prints an error else it prints the help string
 * found inside the struct. When no arguments are passed it behaves like
 * 'help help' was called by setting the code to HELP_CMD. When the code is
 * HELP_CMD a list of all available commands is also printed.
 */
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

/**
 * @brief free all memory allocated by the process structs.
 *
 * usually called by shell_exit(). Pointer p is used to iterate through the list,
 * pr is used to free the last element. Once p is NULL, pr frees the last element
 * of the list.
 */
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

/**
 * @brief free memory and call exit.
 * @param argc If argc==2 a custom exit code was passed.
 * @param argv when empty, the default exit code (0) is used. Else, the code bassed as an argument is used.
 *
 *
 */
void shell_exit(int argc, char** argv)
{
    int exit_code = 0;
    if (argc > 2) PRINT_BAD_ARGS_MSG(argv[0]);
    if (argc == 2) exit_code = atoi(argv[1]);

    free_all();
    if (save_history_to_file) write_history(NULL);
    exit(exit_code);
}


/**
 * @brief Print a list of all active jobs. Theoritically including the foreground one.
 * @param argc unused.
 * @param argv unused.
 */
void jobs_list(int argc, char** argv)
{
    process *p;

    if (argc > 1) PRINT_BAD_ARGS_MSG(argv[0]);

    for (p = head; p != NULL; p = p->next) {
        if (p->pid) {
            /* ignore the master process */
            printf("[%d] %s",
                   p->pid,
                   (p->completed) ? "COMPLETED" : "RUNNING");
            if (p->completed) {
                /* currently this should never happen. */
                printf(" status: %d\n", p->status);
            } else printf("\n");
        }
    }
}

/**
 * @brief Change the current working directory
 * @param argc any value >1 for a specific directory, 1 for home directory.
 * @param argv if argc>1 argv[1] and onwards contains the target directory.
 *
 * argv is splitted on spaces so we must reconstruct the string to get the real
 * target directory. This is not an ideal behavior, spaces in directory names
 * should be escaped and not passed normally, but the current input parser doesn't
 * allow this.
 *
 * The total size neeeded to be allocated is calculated with strlen() and the final
 * string is created with strcat() calls.
 */
void change_directory(int argc, char **argv)
{
    if (argc == 1) {
        /* no arguments after 'cd', change directory to HOME */
        chdir(getenv("HOME"));
    } else {
        /* Merge all argv[] strings together => handle paths with spaces in them */
        /* 0 because:
         * strlen() + 1 gives an extra '1' for at argc-1 => +1 extra
         * we must include the null terminator but strlen doesn't include it => -1 */
        size_t total_size = 0;  /*size in bytes that needs to be allocated */
        char *full_dir; /* final result */
        int i;
        /* start from argv[1] and calculate the total size with strlen() */
        for (i=1; i<argc; ++i) total_size += (strlen(argv[i]) + 1) * sizeof(char);
        full_dir = malloc(total_size); /* allocate the calculated size */
        strcpy(full_dir, argv[1]);     /* strcpy the first string */
        for (i=2; i<argc; ++i)
        {
            strcat(full_dir, " ");     /* use strcat() to append a space */
            strcat(full_dir, argv[i]); /* use strcat() to append the next string */
        }
        /* call the chdir() command and detect errors */
        if ((chdir(full_dir) ) == -1)
        {
            /* error in chdir */
            perror(argv[0]);
        }
        free(full_dir);
    }
}

/**
 * @brief call the specified builtin function.
 * @param code the code of the target builtin function.
 * @param argc argument count to be passed.
 * @param argv argument vector to be passed.
 *
 *
 */
void call_builtin(int code, int argc, char** argv)
{
    builtins[code].action(argc, argv);
}

/**
 * @brief check if a string is a builtin.
 * @param cmd the string to be checked.
 * @returns -1 if the command was not found. The code of the builtin if the command was found.
 */
int check_if_builtin(char *cmd)
{
    int code;
    for (code = 0; code < BUILTINS_NUM; ++code) {
        if (strcmp(builtins[code].cmd, cmd) == 0) return code;
    }
    return -1;
}
