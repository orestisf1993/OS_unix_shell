#ifndef SHELL_UTILS
#define SHELL_UTILS
void init();
int check_builtins(char *cmd);
#endif

/* a single process.  */
typedef struct process
{
  struct process *next;   /* next process */
  char **argv;            /* for execvp */
  pid_t pid;              /* process ID */
  char completed;         /* true if process has completed */
  char stopped;           /* true if process has stopped */
  int status;             /* reported status value */
} process;
