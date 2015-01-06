#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
//~ #include <sys/types.h>
#include <sys/wait.h>

//~ #include <readline/readline.h>
//~ #include <readline/history.h>
//~ #include <sys/time.h>

#define MAX_LENGTH 1024
#define MAX_ARGS 5

#define TEMP_PATH "/usr/bin/"

char **cmd_argv;

int main(int argc, char *argv[])
{
    char line[MAX_LENGTH];
    char *r;
    r = malloc(MAX_LENGTH * sizeof(char));
    int n = 0;
    char *args[MAX_ARGS];

    while (1) {
        printf("YOLOTERM$");
        if (!fgets(line, MAX_LENGTH, stdin)) break;

        n = 0;
        int lengths[MAX_ARGS] = {0};
        int total_len=0;
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

        char *cmd = malloc((sizeof("/bin/") + lengths[0])*sizeof(char));
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
