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

#define MAX_LENGTH 1024
#define MAX_ARGS 5

char cwd[PATH_MAX];
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

/* FreeBSD does not have HOST_NAME_MAX defined.
 * TODO: use sysconf() to discover its value. */
#if !defined(HOST_NAME_MAX)
#if defined(_POSIX_HOST_NAME_MAX)
#define HOST_NAME_MAX _POSIX_HOST_NAME_MAX
#define HOST_SRC "HOST_NAME_MAX"

#elif defined(PATH_MAX)
#define HOST_NAME_MAX PATH_MAX
#define HOST_SRC "PATH_MAX"

#elif defined(_SC_HOST_NAME_MAX)
#define HOST_NAME_MAX _SC_HOST_NAME_MAX
#define HOST_SRC "_SC_HOST_NAME_MAX"

#else
#define HOST_NAME_MAX 256
#define HOST_SRC "256"
#endif
#endif

char hostname[HOST_NAME_MAX + 1];
#undef HOST_NAME_MAX
void print_host()
{
    if(gethostname(hostname, sizeof(hostname)) == 0)
        printf("%s", hostname);
    else printf("(hostname failed?)");
}

int main(/*int argc, char *argv[]*/)
{
    char line[MAX_LENGTH];
    int n = 0;
    /* args is an array of strings where args from strtok are passed */
    char *args[MAX_ARGS];
    char *r;
    int pid;
    int lengths[MAX_ARGS];
    int i;
    int total_len;
    char **paths = NULL;
    int path_count = 0;
    char *path_variable;
    /* welcoming message */
    //TODO: print welcoming message

    init();
    r = malloc(PATH_MAX * sizeof(char));
    path_variable = malloc(PATH_MAX * sizeof(char));

    strcpy(path_variable, getenv("PATH"));
    while((r = strtok(path_count ? NULL : path_variable, ":")) != NULL) {
        paths = (char **)realloc(paths, (path_count + 1) * sizeof(char *));
        paths[path_count++] = r;
    }

    for (i = 0; i < path_count; ++i) printf("path %d: %s\n", i, paths[i]);

    free(path_variable);
    r = realloc(r, MAX_LENGTH * sizeof(char));
    while (1) {
        print_user();
        printf("@");
        print_host();
        printf(":");
        print_cwd();
        printf("$ ");

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
        args[n] = NULL;

        //TODO: check if command is a builtin
        /* check if command is a builtin
         * args[0] currently holds the 'main' command */
        if (check_builtins(args[0]) >= 0) {
            printf("built in!\n");
            continue;
        } else {}

        pid = fork();
        if (pid == -1) {
            perror("fork");
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
            //~ printf("exit status: %d\n", WEXITSTATUS(status));
            if (WIFSIGNALED(status)) {
                /* child process was terminated by a signal
                 * print to stderr the termination signal message */
                psignal(WTERMSIG(status), args[0]);
            }
        }
    }
    return 0;
}
