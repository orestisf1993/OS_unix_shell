#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <signal.h>
#include <sys/wait.h>
#include <pwd.h>

#include "utils.h"

#include <readline/readline.h>
#include <readline/history.h>

int interrupt_called = 0;

char* shell_get_cwd()
{
    char *cwd = malloc(PATH_MAX);
    if(getcwd(cwd, PATH_MAX)) return cwd;
    else {
        perror("cwd");
        return NULL;
    }
}

char* shell_get_host()
{
    char *hostname = malloc(HOST_NAME_MAX);
    if(gethostname(hostname, HOST_NAME_MAX) == 0)
        return hostname;
    else {
        perror("gethostname");
        return NULL;
    }
}

char* shell_get_user()
{
    struct passwd *p = getpwuid(getuid());
    if (!p) {
        perror("getuid");
        return NULL;
    } else return p->pw_name;
}

void create_prompt_message(char **buffer)
{
    size_t needed;
    char *host;
    char *user;
    char *cwd;

    user = shell_get_user();
    host = shell_get_host();
    cwd = shell_get_cwd();
    needed = snprintf(NULL, 0, "%s@%s:%s$ ", user, host, cwd);
    *buffer = realloc(*buffer, needed + 1);
    sprintf(*buffer, "%s@%s:%s$ ", user, host, cwd);

    free(host);
    free(cwd);
}

void interrupt_handle()
{
    printf("\n");
    fflush(stdin);
    fflush(stdout);
    interrupt_called = 1;
}

void mass_signal_set(int handler_code)
{
    switch(handler_code) {
        case SET_DFL:
            signal (SIGINT,  SIG_DFL);
            signal (SIGQUIT, SIG_DFL);
            signal (SIGTSTP, SIG_DFL);
            signal (SIGTTIN, SIG_DFL);
            signal (SIGTTOU, SIG_DFL);
        case SET_IGN:
            signal (SIGINT,  interrupt_handle);
            signal (SIGQUIT, SIG_IGN);
            signal (SIGTSTP, SIG_IGN);
            signal (SIGTTIN, SIG_IGN);
            signal (SIGTTOU, SIG_IGN);
    }
}

int check_background(char *s)
{
    /* if the last char is an ampersand replace it with '\0' */
    int i = 0;
    while(1) {
        if (s[i] == '\0') return -1;
        else if (s[i] == '&') {
            s[i] = 0;
            return i;
        }
        i++;
    }
}

process *pop_from_pid(pid_t id_to_match)
{
    /* returns a pointer to the process tha matches the given id */
    process *p;
    process *previous;

    previous = head;
    if (previous->pid == id_to_match) {
        head = previous->next;
        return previous;
    }

    for (p = head->next; p != NULL; previous = p, p = p->next) {
        if (p->pid == id_to_match) {
            printf("PR_NEXT: %d P_NEXT: %d\n", previous->next->pid, p->next->pid);
            previous->next = p->next;
            printf("AFTER: %d\n", previous->next->pid);
            return p;
        }
    }
    return NULL;
}

int always_print_dead = 0;
void harvest_dead_child()
{
    /* This handler is called once a child process that was running in the background dies.
     * It will only find and mark as complete one process from the linked list */
    process *p;
    pid_t target_id;
    int status;

    while ((target_id = waitpid(-1, &status, WNOHANG)) < 0) {}
    if (WIFSIGNALED(status)) {
        /* child process was terminated by a signal
         * print to stderr the termination signal message */
        char msg[SIGNAL_MSG_LENGTH];
        sprintf(msg, "[%d] exited with status %d", target_id, status);
        psignal(WTERMSIG(status), msg);
    }
    else if(always_print_dead) printf("[%d] exited with status %d\n", target_id, status);

    if ((p = pop_from_pid(target_id)) == NULL)
        fprintf(stderr, "ERROR: terminated child not found in process linked list\n");
    else {
        p->completed = 1;
        p->status = status;
        if (p->bg)
        {
            free(p); /* don't free fg processes, main() does it */
            if (!always_print_dead && !WIFSIGNALED(status)){
                printf("[%d] exited with status %d\n", target_id, status);
            }
        }
    }
}

void parse_path()
{
    char *path_variable;
    char *r;
    char **paths = NULL;
    int path_count = 0;

    r = malloc(PATH_MAX * sizeof(char));
    path_variable = malloc(PATH_MAX * sizeof(char));

    strcpy(path_variable, getenv("PATH"));
    while((r = strtok(path_count ? NULL : path_variable, ":")) != NULL) {
        paths = (char **)realloc(paths, (path_count + 1) * sizeof(char *));
        paths[path_count++] = r;
    }

    /* do stuff with path or return it */
    while (path_count--) free(paths[path_count]);
    free(r);
    free(path_variable);
}

void continue_clear(char **line)
{
    interrupt_called = 0;
    free(*line);
    *line = NULL;
}

void welcoming_message()
{
    printf("\n");
    printf("------------------------------------\n");
    printf("Aristotle University of Thessaloniki\n");
    printf("School of Electrical and Computer Engineering\n");
    printf("\n");
    printf("Creating a Unix shell assignment for \"Operating Systems\" course (7th semester)\n");
    printf("Creator: Orestis Floros-Malivitsis\n");
    printf("----------------------------------\n");
    printf("Ctrl-D or 'exit' to exit\n");
    printf("Type 'help [cmd]' for help on a specific command\n");
    printf("Type 'help' for a list of all available commands\n");
    printf("\n");
    printf("This shell uses by default a history log located at $HOME/.history\n");
    printf("Use the UP/DOWN arrow keys to navigate through history\n");
    printf("Type hoff if you want to disable the history log file\n");
    printf("Type hon if you want to reenable the history log file\n");
    printf("Type pdead [on|off] to enable/disable printing the status of foreground processes on their death (always on for background processes)\n");
    printf("----------------------------------------------------\n");
    printf("\n");
}

int main(/*int argc, char *argv[]*/)
{
    process *current;
    char *line;
    int builtin_code;
    int n = 0;
    /* args is an array of strings where args from strtok are passed.
     * we don't need to call free() on args array elements. */
    char *args[ARGS_ARRAY_LEN];
    pid_t pid;
    int run_background;
    sigset_t mask;
    char *line_leftover = NULL;
    char *prompt_buffer = NULL;

    /* initialize the new signal mask */
    sigemptyset(&mask);
    sigdelset(&mask, SIGCHLD);

    /* welcoming message */
    welcoming_message();

    /* master process entry */
    head = malloc(sizeof(process));
    head->pid = 0;
    head->next = NULL;
    current = head;

    /* handle child death */
    signal(SIGCHLD, harvest_dead_child);
    rl_getc_function = getc;

    read_history(NULL);

    while (1) {
        mass_signal_set(SET_IGN);

        create_prompt_message(&prompt_buffer);

        line = line_leftover ? line_leftover : readline(prompt_buffer);
        if (line == NULL) {
            if (!interrupt_called) shell_exit(0, NULL);
        } else if (strcmp(line, "") == 0) {
            continue_clear(&line);
            continue;
        }

        if (interrupt_called) {
            continue_clear(&line);
            continue;
        } else add_history(line);

        /* check if process should be run in the background before we edit 'line' */
        if ((run_background = check_background(line)) == -1) {
            run_background = 0;
            line_leftover = NULL;
        } else {
            if(line[run_background + 1]) line_leftover = strdup(&line[run_background + 1]);
            run_background = 1;
        }

        n = 0;
        /* WARNING! strtok modifies the initial string */
        args[n++] = strtok(line, " \n");
        while ((args[n++] = strtok(NULL, " \n")) && n < MAX_ARGS) {}

        /* check if command is a builtin
         * args[0] currently holds the 'main' command */
        if ((builtin_code = check_if_builtin(args[0])) >= 0) {
            if (run_background) fprintf(stderr, "WARNING: builtin commands cannot be run in the background\n");
            call_builtin(builtin_code, n - 1, args);
            continue_clear(&line);
            continue;
        } else {}

        current = malloc(sizeof(process));
        current->completed = 0;
        current->bg = run_background;
        current->next = head;
        head = current;

        pid = current->pid = fork();
        if (pid == -1) {
            perror("fork");
            exit(1);
        } else if (pid == 0) {
            /* child */
            mass_signal_set(SET_DFL);
            if (execvp(args[0], args) < 0) {
                /* execv returns error */
                perror(args[0]);
                _exit(EXIT_FAILURE);   /* exec never returns */
            }
        } else {
            /* parent */
            if (!run_background) {
                /* foreground process */
                /* This is NOT a race condition:
                 * if the child process dies before this point of the code is reached,
                 * current->complete is already TRUE because harvest_dead_child()
                 * has already been called and the while loop is never executed */
                while(!(current->completed)) {
                    /* suspend until SIGCHLD signal is received
                     * but does not block the signal, so the handler is executed normally */
                    /*sigsupsend always returns -1 */
                    sigsuspend(&mask);
                }
                free(current);
            } else
            {
                setpgid(pid, pid); /* theoretically a race condition but it only affects interruption signals for some nanoseconds. */
                printf("[%d] started\n", pid);
            }
        }
        continue_clear(&line);
    }
    return 0;
}
