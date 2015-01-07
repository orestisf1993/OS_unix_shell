#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <signal.h>
#include <limits.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <pwd.h>

#include "utils.h"

//TODO: remove all '//' comments
//~ #include <readline/readline.h>
//~ #include <readline/history.h>
//~ #include <sys/time.h>


void print_cwd()
{
    if (getcwd(cwd, sizeof(cwd)))
        printf("%s", cwd);
    else
        printf("(cwd failed?)");
}

void print_user()
{
    struct passwd *p = getpwuid(getuid());
    if (p) printf("%s", p->pw_name);
    else printf("(uid failed?)");
}

void print_host()
{
    if(gethostname(hostname, sizeof(hostname)) == 0)
        printf("%s", hostname);
    else printf("(hostname failed?)");
}

void print_prompt()
{
    print_user();
    printf("@");
    print_host();
    printf(":");
    print_cwd();
    printf("$ ");
}

void mass_signal_set(__sighandler_t handler)
{
    /* sets all interactive and job-control signals to a specific value */
    signal (SIGINT,  handler);
    signal (SIGQUIT, handler);
    signal (SIGTSTP, handler);
    signal (SIGTTIN, handler);
    signal (SIGTTOU, handler);
    signal (SIGCHLD, handler);
}

int check_background(char *s)
{
    /* if the last char is an ampersand replace it with '\0' */
    int last = strlen(s) - 1;
    if (s[last] == '&') {
        s[last] = 0;
        return 1;
    } else return 0;
}

void list_all()
{
    process *p;
    printf("pid completed status\n");
    for (p = head; p != NULL; p = p->next) {
        printf("%d", p->pid);
        if (p->pid) printf(" %d %d", p->completed, p->status);
        else printf(" MASTER MASTER");
        printf("\n");
    }
}

int main(/*int argc, char *argv[]*/)
{
    process *current;
    char line[MAX_LENGTH];
    int n = 0;
    /* args is an array of strings where args from strtok are passed */
    char *args[MAX_ARGS + 1];
    char *r;
    pid_t pid;
    //TODO: delete lengths + total_len because they are pretty much useless
    int lengths[MAX_ARGS];
    int i;
    int total_len;
    int run_background;
    char **paths = NULL;
    int path_count = 0;
    char *path_variable;
    /* welcoming message (?)*/
    //TODO: print welcoming message

    init();
    r = malloc(PATH_MAX * sizeof(char));
    path_variable = malloc(PATH_MAX * sizeof(char));

    strcpy(path_variable, getenv("PATH"));
    while((r = strtok(path_count ? NULL : path_variable, ":")) != NULL) {
        paths = (char **)realloc(paths, (path_count + 1) * sizeof(char *));
        paths[path_count++] = r;
    }

    //~ for (i = 0; i < path_count; ++i) printf("path %d: %s\n", i, paths[i]);

    free(path_variable);
    r = realloc(r, MAX_LENGTH * sizeof(char));

    master.pid = 0;
    master.next = NULL;
    current = &master;
    head = current;

    //~ for(i = 0; i < 5; ++i) {
        //~ current = malloc(sizeof(process));
        //~ current->pid = i + 5;
        //~ current->completed = 1;
        //~ current->status = (i + 20) * 5;
        //~ current->next  = head;
        //~ head = current;
    //~ }
    //~ list_all();

    while (1) {
        print_prompt();

        //TODO: use gnu readline
        if (!fgets(line, MAX_LENGTH, stdin)) break;

        n = 0;
        for (i = 0; i < MAX_ARGS; ++i) lengths[i] = 0;
        total_len = 0;
        /* WARNING! strtok modifies the initial string */
        r = strtok(line, " \n");
        args[n++] = r;

        while ((r = strtok(NULL, " \n")) && n < MAX_ARGS) {
            lengths[n] += strlen(r);
            total_len += lengths[n];

            args[n++] = r;
        }


        //TODO: check if command is a builtin
        /* check if command is a builtin
         * args[0] currently holds the 'main' command */
        if (check_builtins(args[0]) >= 0) {
            printf("built in!\n");
            continue;
        } else {}

        if ((run_background = check_background(args[n - 1]))) {
            /* if args[n-1] was only an '&', we dont' need the string */
            if (args[n - 1][0] == 0) n--;
        }
        args[n] = NULL;

        pid = fork();
        if (pid == -1) {
            perror("fork");
            exit(1);
        } else if (pid == 0) {
            /* child */

            if (execvp(args[0], args) < 0) {
                /* execv returns error */
                //~ fprintf(stderr, "%s: %s\n", args[0], strerror(errno));
                perror(args[0]);
                _exit(EXIT_FAILURE);   /* exec never returns */
            }
        } else {
            /* parent */

            int status;
            waitpid(pid, &status, 0);
            if (WIFSIGNALED(status)) {
                /* child process was terminated by a signal
                 * print to stderr the termination signal message */
                psignal(WTERMSIG(status), args[0]);
            }

            current = malloc(sizeof(process));
            current->pid = pid;
            current->completed = 1;
            current->status = status;
            current->next  = head;
            head = current;
            list_all();
        }
    }
    return 0;
}
