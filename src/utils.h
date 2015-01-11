#ifndef SHELL_UTILS
#define SHELL_UTILS

#include <limits.h>
#include <sys/types.h>

#ifndef sig_t
#ifdef __sighandler_t
#define sig_t __sighandler_t
#elif defined(sighandler_t)
#define sig_t sighandler_t
#endif
#endif

void init_builtins();
int check_builtins(char *cmd);


#define MAX_LENGTH 1024
#define MAX_ARGS 5
#define SIGNAL_MSG_LENGTH 100


enum {SET_DFL = 0,
      SET_IGN
     } signal_set;

void shell_exit();
void list_all();

/* a single process. */
typedef struct process {
    struct process *next;     /* next process */
    pid_t pid;                /* process ID */
    int completed;           /* true if process has completed */
    int status;               /* reported status value */
    int bg;                 /* true if process is running on the background */
} process;

typedef struct cleanup {
    char *r;
} cleanup;

process *head;

char cwd[PATH_MAX];

/* define HOST_NAME_MAX if it is not already defined */
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
#endif
