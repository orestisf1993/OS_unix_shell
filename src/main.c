#define _BSD_SOURCE
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
//~ #include <sys/types.h>
#include <sys/wait.h>
#include <pwd.h>

//~ #include <readline/readline.h>
//~ #include <readline/history.h>
//~ #include <sys/time.h>

#define MAX_LENGTH 1024
#define MAX_ARGS 5

//TODO: use real path ($PATH ??) for execution
#define TEMP_PATH "./"
//~ #define TEMP_PATH "/usr/bin/"

char cwd[1024];
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
#if !defined(HOST_NAME_MAX) && defined(_POSIX_HOST_NAME_MAX)
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
    //TODO: Delete and print real message
    printf("hi! host src:"HOST_SRC"\n"); //DEL
    char line[MAX_LENGTH];
    char *r;
    r = malloc(MAX_LENGTH * sizeof(char));
    int n = 0;
    char *args[MAX_ARGS];

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
        int lengths[MAX_ARGS] = {0};
        int total_len = 0;
        /* WARNING! strtok modifies the initial string */
        r = strtok(line, " \n");
        args[n++] = r;
        while ((r = strtok(NULL, " \n")) && n < MAX_ARGS) {
            lengths[n] += strlen(r);
            total_len += lengths[n];

            args[n++] = r;
        }
        args[n] = NULL;

        char *cmd = malloc((sizeof("/bin/") + lengths[0]) * sizeof(char));
        sprintf(cmd, TEMP_PATH"%s", args[0]);
        //~ printf("final command: %s\nresult:\n", cmd);

        int pid = fork();
        if (pid == -1) {
            fprintf(stderr, "ERROR FORKING!\n");
        } else if (pid == 0) {
            /* child */
            if (execv(cmd, args) < 0) {
                /* execv returns error */
                //~ fprintf(stderr, "%s: %s\n", args[0], strerror(errno));
                perror("ls");
                exit(EXIT_FAILURE);   /* exec never returns */
            }
        } else {
            /* parent */
            int status;
            waitpid(pid, &status, 0);
        }
    }
    return 0;
}
