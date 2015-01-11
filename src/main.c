#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <signal.h>
#include <sys/wait.h>
#include <pwd.h>

#include "utils.h"

//TODO: remove all '//' comments
#include <readline/readline.h>
#include <readline/history.h>
//~ #include <sys/time.h>

int interrupt_called = 0;

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

//TODO: move to builtins.c, rename and use the format used by the 'jobs' bash command to print non-completed jobs
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

process *get_from_pid(pid_t id_to_match)
{
    /* returns a pointer to the process tha matches the given id */
    process *p;
    for (p = head; p != NULL; p = p->next) {
        if (p->pid == id_to_match) return p;
    }
    return NULL;
}

void harvest_dead_children()
{
    //TODO: delete instead of marking as complete
    /* This handler is called once a child process that was running in the background dies.
     * It will only find and mark as complete one process from the linked list */
    process *p;
    pid_t target_id;
    int status;

    printf("harvest called! \n");

    while ((target_id = waitpid(-1, &status, WNOHANG)) < 0) {}
    if (WIFSIGNALED(status)) {
        /* child process was terminated by a signal
         * print to stderr the termination signal message */
        char msg[SIGNAL_MSG_LENGTH];
        sprintf(msg, "[%d] exited with status %d", target_id, status);
        psignal(WTERMSIG(status), msg);
    } else printf("[%d] exited with status %d\n", target_id, status);

    if ((p = get_from_pid(target_id)) == NULL)
        fprintf(stderr, "ERROR: terminated child not found in process linked list\n");
    else {
        p->completed = 1;
        p->status = status;
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

void continue_clear(char *line)
{
    interrupt_called = 0;
    free(line);
}

int main(/*int argc, char *argv[]*/)
{
    process *current;
    //~ char line[MAX_LENGTH];
    char *line;
    int n = 0;
    /* args is an array of strings where args from strtok are passed */
    char *args[MAX_ARGS + 2];
    pid_t pid;
    int run_background;
    sigset_t mask;
    char *line_leftover = NULL;

    /* initialize the new signal mask */
    sigemptyset(&mask);
    sigdelset(&mask, SIGCHLD);

    /* welcoming message (?)*/
    //TODO: print welcoming message

    init_builtins();

    /* master process entry */
    current = malloc(sizeof(process));
    current->pid = 0;
    current->next = NULL;
    head = current;

    /* handle child death */
    signal(SIGCHLD, harvest_dead_children);
    rl_getc_function = getc;
    //~ rl_clear_signals();

    read_history(NULL);

    while (1) {
        mass_signal_set(SET_IGN);

        //~ print_prompt();

        //TODO: history, history_write(), history_read() + msg
        //TODO: prompt
        line = line_leftover ? line_leftover : readline("shell: ");
        //TODO: EXITCODES
        if (line == NULL) {
            if (!interrupt_called) shell_exit();
        } else if (strcmp(line, "") == 0) {
            continue_clear(line);
            continue;
        }

        if (interrupt_called) {
            continue_clear(line);
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
        //TODO: handle builtin commands
        if (check_builtins(args[0]) >= 0) {
            continue_clear(line);
            continue;
        } else {}

        current = malloc(sizeof(process));
        current->completed = 0;
        current->next = head;
        head = current;

        //TODO: fix memleak of args and line

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
                 * current->complete is already TRUE because harvest_dead_children()
                 * has already been called and the while loop is never executed */
                while(!(current->completed)) {
                    /* suspend until SIGCHLD signal is received
                     * but does not block the signal, so the handler is executed normally */
                    /*sigsupsend always returns -1 */
                    sigsuspend(&mask);
                }
            } else setpgid(pid, pid);

            //TODO: del this
            list_all();
        }
        continue_clear(line);
    }
    //TODO: free some memory ;)
    return 0;
}
