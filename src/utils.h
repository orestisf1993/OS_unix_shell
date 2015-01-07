#ifndef SHELL_UTILS
#define SHELL_UTILS
void init();
int check_builtins(char *cmd);
#endif

#define MAX_LENGTH 1024
#define MAX_ARGS 5

/* a single process.  */
typedef struct process
{
  struct process *next;   /* next process */
  char *args[MAX_ARGS + 1];            /* for execvp */
  pid_t pid;              /* process ID */
  char completed;         /* true if process has completed */
  char stopped;           /* true if process has stopped */
  int status;             /* reported status value */
} process;


char cwd[PATH_MAX];

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
