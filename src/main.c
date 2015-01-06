#define _BSD_SOURCE
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
//~ #include <sys/types.h>
#include <sys/wait.h>
#include <pwd.h>

//~ #include <readline/readline.h>
//~ #include <readline/history.h>
//~ #include <sys/time.h>

#define MAX_LENGTH 1024
#define MAX_ARGS 5

#define TEMP_PATH "/usr/bin/"

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
    printf("hi! host src:"HOST_SRC"\n");
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
        printf("$");
        
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

        //~ for (int i = 0; i < n; ++i) printf("%s ", args[i]);
        //~ printf("\n");

        char *cmd = malloc((sizeof("/bin/") + lengths[0]) * sizeof(char));
        sprintf(cmd, TEMP_PATH"%s", args[0]);
        //~ printf("final command: %s\nresult:\n", cmd);

        int pid = fork();
        int status;
        switch (pid) {
            case -1:
                printf("ERROR FORKING!\n");
                break;
            case 0:
                /* child */
                execv(cmd, args);
                exit(EXIT_FAILURE);   /* exec never returns */
            default:
                /* parent */
                waitpid(pid, &status, 0);
        }

    }
    return 0;
}
