#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

//~ #include <readline/readline.h>
//~ #include <readline/history.h>
//~ #include <sys/time.h>

#define MAX_LENGTH 1024

char **cmd_argv;

int main(int argc, char *argv[])
{
    //~ char line[MAX_LENGTH];
    //~ char *r;
    //~ r = malloc(MAX_LENGTH * sizeof(char));
    
    char **lineptr;
    char *line;

    int oc;             /* option character */

    while ((oc = getopt(argc, argv, "ab::")) != -1) {
        switch(oc){
            case 'a':
                printf("a\n");
                break;
            case 'b':
                printf("b->%s\n", optarg);
                break;
        }
    }

    //~ while (1) {
        //~ printf("$ ");
        //~ if (!fgets(line, MAX_LENGTH, stdin)) break;
        //~ /* using 'system' is cheating since it calls /bin/bash
         //~ * maybe keep the base code and write our own 'system' func*/
        //~ /* WARNING! strtok modifies the initial string */
        //~ printf("%s", strtok(line, " -"));
        //~ while ((r = strtok(NULL, " -")) != NULL)
            //~ printf("%s\n", r);
//~ 
    //~ }

    //~ while(1){
        //~ size_t n = 0;
        //~ getline(lineptr, &n, stdin);
        //~ line = *lineptr;
        //~ printf("%s\n%lu",line, n);
    //~ }

    return 0;
}
