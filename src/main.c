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

//TODO: Rename
void ctrl_c_handle()
{
    printf("\n");
    fflush(stdin);
    fflush(stdout);
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
    //TODO: test function for multiple children deaths
    //TODO: delete instead of marking as complete
    /* This handler is called once a child process that was running in the background dies.
     * It will only find and mark as complete one process from the linked list */
    process *p;
    pid_t target_id;
    int status;

    printf("harvest called! \n");

    //~ if ((target_id = waitpid(-1, &status, WNOHANG)) < 0) return;

    //TODO: check if it is a race
    while ((target_id = waitpid(-1, &status, WNOHANG)) < 0) {}
    /* if a foreground process dies the SIGCHLD is send but it has already been handled*/
    //TODO: remove waitpid from the main loop and then change the above comment

    if (WIFSIGNALED(status)) {
        /* child process was terminated by a signal
         * print to stderr the termination signal message */
        psignal(WTERMSIG(status), NULL);
    }

    printf("target_id=%d status=%d\n", target_id, status);
    printf("process %d died ", target_id);
    if ((p = get_from_pid(target_id)) == NULL)
        fprintf(stderr, "ERROR: terminated child not found in process linked list\n");
    else {
        printf(" check: %d\n", p->pid);
        p->completed = 1;
        p->status = status;
    }

}

int main(/*int argc, char *argv[]*/)
{
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

    sigset_t mask;

    /* initialize the new signal mask */
    sigemptyset(&mask);
    sigdelset(&mask, SIGCHLD);


    //~ sigemptyset (&mask);
    //~ sigaddset (&mask, SIGCHLD);
    //~ if (sigprocmask(SIG_UNBLOCK, &mask, &orig_mask) < 0) {
    //~ perror ("sigprocmask");
    //~ return 1;
    //~ }

    /* welcoming message (?)*/
    //TODO: print welcoming message

    init();
    //TODO: delete path analyzing or find a use for it
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

    /* handle child death */
    signal(SIGCHLD, harvest_dead_children);

    while (1) {
        mass_signal_set(SIG_IGN);
        print_prompt();

        //TODO: use gnu readline
        //TODO: handle empty line
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

        /* check if command is a builtin
         * args[0] currently holds the 'main' command */
        //TODO: handle builtin commands
        if (check_builtins(args[0]) >= 0) {
            printf("built in!\n");
            continue;
        } else {}

        if ((run_background = check_background(args[n - 1]))) {
            /* if args[n-1] was only an '&', we dont' need the string */
            if (args[n - 1][0] == 0) n--;
        }
        args[n] = NULL;

        current = malloc(sizeof(process));
        current->completed = 0;
        current->next = head;
        head = current;

        //TODO: fix memleak

        pid = current->pid = fork();
        if (pid == -1) {
            perror("fork");
            exit(1);
        } else if (pid == 0) {
            /* child */
            mass_signal_set(SIG_DFL);
            if (execvp(args[0], args) < 0) {
                /* execv returns error */
                //~ fprintf(stderr, "%s: %s\n", args[0], strerror(errno));
                perror(args[0]);
                _exit(EXIT_FAILURE);   /* exec never returns */
            }
        } else {
            /* parent */
            //TODO: point current to a tmp process and update it without malloc for a more generic code
            int status;
            int res;

            //~ sleep(1);

            list_all();

            //TODO: fix this, race condition
            //~ res = waitpid(pid, &status, run_background ? WNOHANG : 0);
            if (!run_background) {
                //~ while(!(current->completed)) {res = sigwaitinfo(&mask, NULL); harvest_dead_children();}
                res = 0; /*sigsupsend always returns -1 */
                while(!(current->completed)) {
                    sigsuspend(&mask);
                }
            } else res = 0;

            //~ if (WIFSIGNALED(status)) {
                //~ /* child process was terminated by a signal
                 //~ * print to stderr the termination signal message */
                //~ psignal(WTERMSIG(status), args[0]);
            //~ }

            //~ current->status = status;
            //~ if (res == -1) fprintf(stderr, "WAITPID ERRROR IN MAIN\n");
            //~ else if (res) current->completed = 1;
            //~ if (res && res != pid) fprintf(stderr, "ERROR: pid=%d waitpid=%d\n", pid, res);

        }
    }
    //TODO: free some memory ;)
    return 0;
}
