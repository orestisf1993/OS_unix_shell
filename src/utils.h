/** \file utils.h
* \brief header file
*
* declares macros related to array lengths, solve some portability problems,
* manage declarations.
*/

#ifndef SHELL_UTILS
#define SHELL_UTILS

#include <limits.h>
#include <sys/types.h>

/* resolve portability problems with defined/undefined macros */
#if !defined(sig_t)
#if defined(__sighandler_t)
#define sig_t __sighandler_t
#elif defined(sighandler_t)
#define sig_t sighandler_t
#endif
#endif

/* define HOST_NAME_MAX if it is not already defined. May solve some portability problems. */
#if !defined(HOST_NAME_MAX)
#if defined(_POSIX_HOST_NAME_MAX)
#define HOST_NAME_MAX _POSIX_HOST_NAME_MAX
#elif defined(PATH_MAX)
#define HOST_NAME_MAX PATH_MAX
#elif defined(_SC_HOST_NAME_MAX)
#define HOST_NAME_MAX _SC_HOST_NAME_MAX
#else
#define HOST_NAME_MAX 256
#endif
#endif

/* builtin related functions */
int check_if_builtin(char *cmd);
void call_builtin(int code, int argc, char **argv);
void free_all();

/* builtin functions */
void jobs_list(int argc, char **argv);
void change_directory(int argc, char **argv);
void shell_exit(int argc, char **argv);
void print_help(int argc, char **argv);
void history_on(int argc, char **argv);
void history_off(int argc, char **argv);
void print_dead(int argc, char **argv);
void print_wd(int argc, char **argv);

/**
 * @brief enum that gives values to all the builtin command codes
 */
enum {
    EXIT_CMD = 0, /**< builtin command code for exit  command*/
    CD_CMD,       /**< builtin command code for cd    command*/
    JOBS_CMD,     /**< builtin command code for jobs  command*/
    HELP_CMD,     /**< builtin command code for help  command*/
    HOFF_CMD,     /**< builtin command code for hoff  command*/
    HON_CMD,      /**< builtin command code for hon   command*/
    PDEAD_CMD,    /**< builtin command code for pdead command*/
    PWD_CMD,      /**< builtin command code for pwd   command*/
    BUILTINS_NUM  /**< length of this enumerator, must always be last */
} builtin_codes_macro;

/**
 * @brief a struct that implements a builtin command.
 */
typedef struct builtin_struct {
    int code;                              /**< builtin command code. One of the values of \enum builtin_codes_macro. */
    char *cmd;                             /**< the command that triggers the action. */
    void (*action)(int argc, char **argv); /**< function pointer to the corresponding builtin function. */
    char *help_text;                       /**< what is printed when 'help [cmd]' is called. */
} builtin_struct;

/**
 * @brief max length of one line.
 */
#define MAX_LENGTH 1024

/**
 * @brief max length of pid as a string.
 */
#define MAX_PID_LENGTH 10

/**
 * @brief the maximum number of arguments the user can input for a command.
 *
 * Does not include argv[0] (command name). If the user enters more arguments they will be ignored.
 * We can overcome this limitation if we implement the argv[] array in main() using a dynamic array.
 *
 * example: user enters:'./t 1 2 3 4 5 6 7' with a MAX_ARGS of 4 ./t will be executed and
 * arguments 1, 2, 3 and 4 will be passed. 5, 6 and 7 will be ignored and will not cause an error.
 */
#define MAX_ARGS 4

/**
 * @brief the real length of *argv[].
 *
 * the *argv[] array needs 2 extra spaces. One for the command name and one for NULL.
 */
#define ARGS_ARRAY_LEN MAX_ARGS + 2

/**
 * @brief Length of the message shown when a child is terminated by a signal.
 */
#define SIGNAL_MSG_LENGTH 100

/**
 * @brief used in mass_signal_set()
 */
enum {
    SET_DFL = 0, /**< Ignore signals */
    SET_IGN      /**< Default behavior */
} signal_set;

/**
 * @brief struct that defines a single process
 */
typedef struct process {
    struct process *next; /**< next process in list. */
    pid_t pid;            /**< process ID. */
    int completed;        /**< true if process is completed. */
    int status;           /**< reported status value. */
    int bg;               /**< true if process is running on the background */
} process;

/** the head process is the latest added process. The head of the linked list */
process *head;

#endif
