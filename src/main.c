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

//TODO: Rename
void ctrl_c_handle()
{
    printf("\n");
    fflush(stdin);
    fflush(stdout);
    interrupt_called = 1;
}

//IDEA: change handler to int and handle with real functions like ctrl_c_handle()
void mass_signal_set(sig_t handler)
{
    /* sets all interactive and job-control signals to a specific value */
    if (handler == SIG_DFL) signal (SIGINT,  handler);
    else signal(SIGINT, ctrl_c_handle);
    signal (SIGQUIT, handler);
    signal (SIGTSTP, handler);
    signal (SIGTTIN, handler);
    signal (SIGTTOU, handler);
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
        sprintf(msg, "[%d]", target_id);
        psignal(WTERMSIG(status), msg);
    }

    printf("[%d] exited with status %d\n", target_id, status);
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

int main(/*int argc, char *argv[]*/)
{
    process *current;
    //~ char line[MAX_LENGTH];
    char *line;
    int n = 0;
    /* args is an array of strings where args from strtok are passed */
    char *args[MAX_ARGS + 2];
    char *r;
    pid_t pid;
    int run_background;
    sigset_t mask;

    /* initialize the new signal mask */
    sigemptyset(&mask);
    sigdelset(&mask, SIGCHLD);

    /* welcoming message (?)*/
    //TODO: print welcoming message

    init_builtins();

    r = malloc(MAX_LENGTH * sizeof(char));

    /* master process entry */
    current = malloc(sizeof(process));
    current->pid = 0;
    current->next = NULL;
    head = current;

    /* handle child death */
    signal(SIGCHLD, harvest_dead_children);
    rl_getc_function = getc;
    rl_clear_signals();

    while (1) {
        mass_signal_set(SIG_IGN);
        interrupt_called = 0; //TODO: move in continue_()
        //~ print_prompt();

        //TODO: history, history_write(), history_read() + msg
        //TODO: prompt
        //TODO: all continue calls call a function to clear memory
        line = readline("shell: ");
        //TODO: EXITCODES
        if (line == NULL) {
            if (!interrupt_called) exit(1);
        } else if (strcmp(line, "") == 0) {
            free(line);
            continue;
        }

        if (interrupt_called) {
            free(line);
            continue;
        } else add_history(line);

        n = 0;
        /* WARNING! strtok modifies the initial string */
        r = strtok(line, " \n");
        args[n++] = r;

        while ((r = strtok(NULL, " \n")) && n < MAX_ARGS) {
            args[n++] = r;
        }

        /* check if command is a builtin
         * args[0] currently holds the 'main' command */
        //TODO: handle builtin commands
        if (check_builtins(args[0]) >= 0) {
            printf("built in!\n");
            //TODO: del
            free(line);
            continue;
        } else {}

        if ((run_background = check_background(args[n - 1]))) {
            /* if args[n-1] was only an '&', we don't need the string */
            if (args[n - 1][0] == 0) n--;
        }
        /* argv[argc] must always be NULL */
        args[n] = NULL;

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
            mass_signal_set(SIG_DFL);
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
            }

            //TODO: del this
            list_all();
        }
        free(line);
    }
    //TODO: free some memory ;)
    return 0;
}
