#include <stdio.h>
#include <stdlib.h>
#include <locale.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include "functions.c"

extern char** environ;

void outputEnv(FILE* file, char**envp, char inpSymb){
    char* buffer = (char*)malloc(sizeof(char) * 40);
    switch (inpSymb){
    
    case '+':
        while (fscanf(file,"%s",buffer) != EOF)
            printf("\n%s=%s",buffer,getenv(buffer));
        break;

    case '*':
        while (fscanf(file,"%s",buffer) != EOF)
            printf("\n%s=%s",buffer,parsEnv(envp,buffer));
        break;

    case '&':
        while (fscanf(file,"%s",buffer) != EOF)
            printf("\n%s=%s",buffer,parsEnv(environ,buffer));
        break;
    }
}
            

int main(int argc,char **argv, char* envp[]){
    printf("\n\nName programm: %s\npid: %d\nppid: %d\n\n",argv[0],getpid(),getppid());
    FILE* file = fopen(argv[1],"r");
    outputEnv(file,envp, *(argv[2]));
    printf("\n\nEnter symbol to exit child programm: ");
    rewind(stdin);
    getchar();
    printf("\n\n\n");
    return 0;
}