/** \file main.c
* \brief the main program.
* 
* contains the main loop in main() and important functions related to input,
* handling processes and signals.
*/

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

/** Sets to True if a user uses ctrl-c (interruption / escape) while typing.
 *  Once set the main loop will skip the current line, free it and reset the prompt.
 *  Default value is False and it is reset at every loop. */
int interrupt_called = 0;

/** pointer to the current process struct */
process *current;   

/**
 * @brief Get the current workind directory.
 * @returns a char* with the current working directory malloced for a length of PATH_MAX.
 * @returns NULL on failure.
 */
char* shell_get_cwd()
{
    char *cwd = malloc(PATH_MAX * sizeof(char));
    if(getcwd(cwd, PATH_MAX * sizeof(char))) return cwd;
    else {
        perror("cwd");
        return NULL;
    }
}

/**
 * @brief Get the current hostname.
 * @returns a char* with the current hostname malloced for HOST_NAME_MAX bytes.
 * @returns NULL on failure.
 */
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

/**
 * @brief Get the current username.
 * @returns a char* with the current username.
 * @returns NULL on failure.
 * 
 * Uses the getpwuid() function that returns a pointer to a structure containing the info that interest us.
 * No need to free anything returned by this function.
 */
char* shell_get_user()
{
    struct passwd *p = getpwuid(getuid());
    if (!p) {
        perror("getuid");
        return NULL;
    } else return p->pw_name;
}

/**
 * @brief Build the prompt message on a buffer.
 * @param buffer the buffer where the output is stored.
 */
void create_prompt_message(char **buffer)
{
    size_t needed;
    char *host;
    char *user;
    char *cwd;

    /* get the strings we need using the shell_get*() functions */
    user = shell_get_user();
    host = shell_get_host();
    cwd = shell_get_cwd();
    /* calculate the bytes we need to allocate using snprintf. */
    needed = snprintf(NULL, 0, "%s@%s:%s$ ", user, host, cwd);
    *buffer = realloc(*buffer, needed + 1);
    sprintf(*buffer, "%s@%s:%s$ ", user, host, cwd);

    /* free host and cwd.
     * No need to free anything from the structure returned by getpwuid() (used by shell_get_user()). */
    free(host);
    free(cwd);
}

/**
 * @brief Handles interrupts (e.g. ctrl-c) that happen while the main process is running without a foreground process active.
 */
void interrupt_handle()
{
    printf("\n");
    fflush(stdin);
    fflush(stdout);
    /* set global variable to True because this function is called on interruption signals */
    interrupt_called = 1;
}

/** the prompt shown when the user is asked to confirm the process kill */
#define KILL_MSG "terminate foreground process with pid %d? (Y/N/a - always)\n"

/** if false the active process is always killed when ctrl-c is pressed */
int ask_to_kill = 1;

/**
 * @brief Handles interrupts (e.g. ctrl-c) that happen while the main process is running with a foreground process active.
 *
 * asks the user to confirm killing the foreground process. If \a ask_to_kill is set to false, the foreground process is always killed.
 */
void killer_interrupt_handle(){
    char *line;
    char msg[sizeof(KILL_MSG) + MAX_PID_LENGTH];
    int no_kill = 1;
    while (ask_to_kill && no_kill)
    {
        sprintf(msg, KILL_MSG, current->pid);
        line = readline(msg);
        if (line==NULL) continue;
        else if (strcasecmp(line, "y") == 0) no_kill = 0;
        else if (strcasecmp(line, "n") == 0) return;
        else if (strcasecmp(line, "a") == 0) ask_to_kill = no_kill = 0;
        else printf("wrong option\n");
    }
    if (current->completed){
        printf("process already dead\n");
    }
    else kill(current->pid, SIGTERM);    
}

/**
 * @brief Sets the behavior for some interrupting signals.
 * @param handler_code set to SET_DFL or SET_IGN.
 */
void mass_signal_set(int handler_code)
{
    switch(handler_code) {
        /* default behavior */
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

/**
 * @brief check if the command inputed by the user must be run in the bacground.
 * @param s is the string that is searched.
 * @returns the position of the ampersand if found.
 * @returns -1 if not found.
 *
 * Search for an ampersand ('&') in the string \a s passed as an argument.
 * If such a character is found replace it with the '\0' character.
 */
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

/**
 * @brief Tries to find a process in the linked list with a specific id.
 * @param id_to_match the id to search for.
 * @returns a pointer to the process that matches the given id.
 * @returns NULL if no process with the given id is found.
 * 
 * Pointer p iterates throughout the linked list.
 * Once it finds the target the previous process is linked with the next.
 */
process *pop_from_pid(pid_t id_to_match)
{
    process *p;         /* Pointer that will iterate throughout the linked list */
    process *previous;  /* Pointer to the process behind p */

    /* start with the head */
    previous = head;
    if (head->pid == id_to_match) {
        /* The head process (1st element in the linked list) has the targeted id.
         * The next process now becomes the head. */
        head = head->next;
        return previous;
    }

    for (p = head->next; p != NULL; previous = p, p = p->next) {
        if (p->pid == id_to_match) {
            previous->next = p->next;
            return p;
        }
    }
    return NULL;
}

/*! If True all processes that die are printed (not only bg). */
int always_print_dead = 0;

/**
 * @brief handle dead processes.
 * 
 * This handler is called once a child process that was running in the background dies.
 * It will only find and mark as complete one process from the linked list.
 */
void harvest_dead_child()
{
    process *p;
    pid_t target_id;
    int status;

    /* waitpid() will find the process that exited. */
    while ((target_id = waitpid(-1, &status, WNOHANG)) < 0) {}
    if (WIFSIGNALED(status)) {
        /* child process was terminated by a signal
         * print to stderr the termination signal message */
        char msg[SIGNAL_MSG_LENGTH];
        sprintf(msg, "[%d] exited with status %d", target_id, status);
        psignal(WTERMSIG(status), msg);
    } else if(always_print_dead) printf("[%d] exited with status %d\n", target_id, status);

    if ((p = pop_from_pid(target_id)) == NULL)
        fprintf(stderr, "ERROR: terminated child not found in process linked list\n");
    else {
        p->completed = 1;
        p->status = status;
        if (p->bg) {
            free(p); /* don't free fg processes, main() does it. */
            if (!always_print_dead && !WIFSIGNALED(status)) {
                printf("[%d] exited with status %d\n", target_id, status);
            }
            /* reset the display once printing is done. */
            rl_forced_update_display();
        }
    }

}

/**
 * @brief parses the PATH environmental variable. Currently useless.
 */
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

/**
 * @brief free memory of line and reset some flags.
 * @param line Pointer to the line string that needs to be freed.
 */
void continue_clear(char **line)
{
    interrupt_called = 0;
    free(*line);
    *line = NULL;
}

/**
 * @brief Print the welcoming message.
 */
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

/**
 * @brief main function containing the main loop.
 * @returns when 1==False...
 *
 * contains the main loop, initializes head, passes arguments to new processes,
 * handles signals, asks for user input through the readline library,
 * keep history, parses user input, splits it with strtok(),
 * checks for builtin commands, forks and handles processes.
 */
int main(/*int argc, char *argv[]*/)
{
    pid_t pid;          /* pid value used in fork() */
    char *line;         /* the current line read */
    int builtin_code;   /* used when a builtin command is detected */
    int argc = 0;       /* number of args, strtok found */
    /* argv is an array of strings where the args from strtok are passed.
     * we don't need to call free() on argv array elements. */
    char *argv[ARGS_ARRAY_LEN];
    int run_background; /* flag set to True when the command needs to be run at the background */
    sigset_t mask;      /* signal mask used by sigsupsend() */
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
        /* Shell shouldn't terminate on signals */
        mass_signal_set(SET_IGN);

        create_prompt_message(&prompt_buffer);
        line = readline(prompt_buffer);
        if (line == NULL) {
            if (!interrupt_called) call_builtin(EXIT_CMD, 1, NULL); /*ctrl-d <=> EOT etc... */
        } else if (strcmp(line, "") == 0) {
            /* empty line. User just pressed 'enter' (?) */
            continue_clear(&line);
            continue;
        }

        if (interrupt_called) {
            /* happens on ctrl-c / interruption. Skip the whole line and don't add it in history log. */
            continue_clear(&line);
            continue;
        } else add_history(line);

        /* check if process should be run in the background before we edit 'line' *
         * run_background holds the value of check_background(). */
        if ((run_background = check_background(line)) == -1) {
            /* not a background_process set the flag to zero.*/
            run_background = 0;
        } else {
            run_background = 1; /* just set to True and use as a flag from now on */
        }

        argc = 0;
        /* WARNING! strtok modifies the initial string */
        /* " \n" is the delimiter. Split the string on ' ' and '\n' although readline() does not include '\n' */
        argv[argc++] = strtok(line, " \n"); /* line must be used as an argument for strtok(), then passing NULL uses line again */
        while ((argv[argc++] = strtok(NULL, " \n")) && argc <= MAX_ARGS) {
            /* Splitting stops once the max number of arguments is reached or once strtok() returns NULL */
        }

        /* check if command is a builtin
         * argv[0] currently holds the 'main' command */
        if ((builtin_code = check_if_builtin(argv[0])) >= 0) {
            if (run_background) fprintf(stderr, "WARNING: builtin commands cannot be run in the background! Ignoring...\n");
            call_builtin(builtin_code, argc - 1, argv);
            continue_clear(&line);
            continue;
        }

        /* allocate space for a new process struct. */
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
            if (execvp(argv[0], argv) < 0) {
                /* execv returns error */
                perror(argv[0]);
                _exit(EXIT_FAILURE);   /* exec shouldn't return */
            }
        } else {
            /* parent */
            setpgid(pid, pid);
            signal(SIGINT, killer_interrupt_handle);
            if (!run_background) {
                /* foreground process, handle child death */
                /* This is NOT a race condition:
                 * if the child process dies before this point of the code is reached,
                 * current->complete is already TRUE because harvest_dead_child() has already been called and the while loop is never executed */
                while(!(current->completed)) {
                    /* suspends until SIGCHLD signal is received
                     * but does not block the signal, so the handler is executed normally
                     * sigsupsend() always returns -1 */
                    sigsuspend(&mask);
                }
                free(current); /* free the finished process. bg processes are freed by harvest_dead_child() */
            } else {
                printf("[%d] started\n", pid);
            }
        }
        continue_clear(&line);
    }
    return -1;
}
