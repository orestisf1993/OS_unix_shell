#ifndef SHELL_UTILS
/**
 * @brief 
 * 
 * 
 */
#define SHELL_UTILS

#include <limits.h>
#include <sys/types.h>

#ifndef sig_t
#ifdef __sighandler_t
/**
 * @brief 
 * 
 * 
 */
#define sig_t __sighandler_t
#elif defined(sighandler_t)
/**
 * @brief 
 * 
 * 
 */
#define sig_t sighandler_t
#endif
#endif

int check_if_builtin(char *cmd);
void call_builtin(int code, int argc, char** argv);

/* builtin functions */
void jobs_list(int argc, char** argv);
void change_directory(int argc, char** argv);
void shell_exit(int argc, char** argv);
void free_all();
void print_help(int argc, char** argv);
void history_on(int argc, char** argv);
void history_off(int argc, char** argv);
void print_dead(int argc, char** argv);
void print_wd(int argc, char** argv);

/**
 * @brief 
 * 
 * 
 */
enum {
    EXIT_CMD = 0, /**<  */
    CD_CMD, /**<  */
    JOBS_CMD, /**<  */
    HELP_CMD, /**<  */
    HOFF_CMD, /**<  */
    HON_CMD, /**<  */
    PDEAD_CMD, /**<  */
    PWD_CMD, /**<  */
    BUILTINS_NUM /* length of this enumerator, must always be last */ /**<  */
} builtin_codes_macro;

/**
 * @brief 
 * 
 * 
 */
typedef struct builtins_struct {
    int code; /**<  */
    char *cmd; /**<  */
    void (*action)(int argc, char** argv);
    char *help_text; /**<  */
/**
 * @brief 
 * 
 * 
 */
} builtins_struct;


/**
 * @brief 
 * 
 * 
 */
#define MAX_LENGTH 1024
/**
 * @brief 
 * 
 * 
 */
#define MAX_ARGS 5
/**
 * @brief 
 * 
 * 
 */
#define ARGS_ARRAY_LEN MAX_ARGS + 2
/**
 * @brief 
 * 
 * 
 */
#define SIGNAL_MSG_LENGTH 100


/**
 * @brief 
 * 
 * 
 */
enum {SET_DFL = 0,
      SET_IGN /**<  */
     } signal_set;

void shell_exit(int argc, char **argv);
void jobs_list(int argc, char **argv);

/* a single process. */
/**
 * @brief 
 * 
 * 
 */
typedef struct process {
    struct process *next;     /* next process */ /**<  */
    pid_t pid;                /* process ID */ /**<  */
    int completed;           /* true if process has completed */ /**<  */
    int status;               /* reported status value */ /**<  */
    int bg;                 /* true if process is running on the background */ /**<  */
/**
 * @brief 
 * 
 * 
 */
} process;

process *head;

/* define HOST_NAME_MAX if it is not already defined */
#if !defined(HOST_NAME_MAX)
#if defined(_POSIX_HOST_NAME_MAX)
/**
 * @brief 
 * 
 * 
 */
#define HOST_NAME_MAX _POSIX_HOST_NAME_MAX
/**
 * @brief 
 * 
 * 
 */
#define HOST_SRC "HOST_NAME_MAX"

#elif defined(PATH_MAX)
/**
 * @brief 
 * 
 * 
 */
#define HOST_NAME_MAX PATH_MAX
/**
 * @brief 
 * 
 * 
 */
#define HOST_SRC "PATH_MAX"

#elif defined(_SC_HOST_NAME_MAX)
/**
 * @brief 
 * 
 * 
 */
#define HOST_NAME_MAX _SC_HOST_NAME_MAX
/**
 * @brief 
 * 
 * 
 */
#define HOST_SRC "_SC_HOST_NAME_MAX"

#else
/**
 * @brief 
 * 
 * 
 */
#define HOST_NAME_MAX 256
/**
 * @brief 
 * 
 * 
 */
#define HOST_SRC "256"
#endif
#endif
#endif
