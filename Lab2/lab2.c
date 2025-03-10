#include <stdio.h>
#include <stdlib.h>
#include <locale.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include "functions.c"

extern char **environ;


void callChild(char* path, int childCount, char* argv[], char** childEnv, char* inpChar){
    pid_t chpid = fork();
        if (chpid == 0){
            char *name = addStrAndNumb("child_",childCount);
            char* args[] = {name, argv[1], inpChar, NULL};
            execve(path, args, childEnv);
            exit(0);
        }
    wait(0);
}



int main(int argc, char*argv[], char*envp[]) {
    setenv("LC_COLLATE", "C", 1);
    char **env = environ;
    int count = 0;
    
    while (NULL != *env) {
        ++count;
        ++env;
    }
    qsort(environ, count, sizeof(char*), compare_strings);
    env = environ;
    while(*env != NULL){
        printf("%s\n", *env);
        ++env;
    }

    count = 0;
    char* childEnv[] = {addStrAndStr("SHELL=",getenv("SHELL")),addStrAndStr("HOME=",getenv("HOME")), addStrAndStr("HOSTNAME=",getenv("HOSTNAME")), 
                    addStrAndStr("LOGNAME=",getenv("LOGNAME")), addStrAndStr("LANG=",getenv("LANG")),
                    addStrAndStr("TERM=",getenv("TERM")), addStrAndStr("USER=",getenv("USER")),
                    addStrAndStr("LC_COLLATE=",getenv("LC_COLLATE")), addStrAndStr("PATH=",getenv("PATH")), NULL};
    while (1){
        printf("\nEnter Symbol: ");
        char symbol = getchar();
        while ( getchar() != '\n' );
        switch (symbol)
        {
        case '+':
            count++;
            callChild(getenv("CHILD_PATH"),count,argv,childEnv, &symbol);
            break;
        case '*':
            count++;
            callChild(parsEnv(envp, "CHILD_PATH"),count,argv,childEnv, &symbol);
            break;
        case '&':
            count++;
            callChild(parsEnv(environ, "CHILD_PATH"),count,argv,childEnv, &symbol);
            break;
        case 'q':
            return 0;
        }
    }
    return 0;
}
