#define _POSIX_SOURCE 
#include <sys/time.h>
#include <stdio.h>
#include <stdlib.h>
#include <locale.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include <stdbool.h>
#include "functions.c"

#define MAX_CHILD 50
extern char **environ;
int childCount = -1;
int childPidMas[MAX_CHILD];

void callChild(char* path, int* childPidMas, int childCount){
    pid_t chpid = fork();
    char *name = addStrAndNumb("child_",childCount);
        if (chpid == 0){
            char *name = addStrAndNumb("child_",childCount);
            char* args[] = {name, NULL};
            execv(path, args);
            perror("execv");
            exit(EXIT_FAILURE);
        }
    printf("\nCREATE CHILD PROCESS: %s", name);
    childPidMas[childCount] = chpid;
}

bool destroyChildProcess(int* childPidMas, int childCount){
    if (childCount != -1){
        kill(childPidMas[childCount],SIGTERM);
        wait(childPidMas + childCount);
        printf("\nCHILD PROCESS %d HAS BEEN DESTROYED", childCount);
        printf("\tThe number of child processes - %d", childCount);
        return true;
    }
    else {
        printf("There are no child processes");
        return false;
    }
}

void listProcesses(int* childPidMas, int childCount){
    printf("\nParent PID: %d", getpid());
    printf("\nChilds PID: ");
    for(int i = 0; i <= childCount; i++){
        printf("%d ", childPidMas[i]);
    }
    printf("\n");
    fflush(stdout);
    char command[20];
    sprintf(command, "pstree -p %d", getpid());
    system(command);
}

void destroyAllChildProcess(int* childPidMas, int childCount){
    for(int i = childCount; i > -1; i--){
        destroyChildProcess(childPidMas, i);
    }
}

void allowAllStats(int* childPidMas, int childCount){
    for(int i = 0; i <= childCount; i++){
        kill(childPidMas[i], SIGUSR1);
    }
}


void banAllStats(int* childPidMas, int childCount){
    for(int i = 0; i <= childCount; i++){
        kill(childPidMas[i], SIGUSR2);
    }
}

void setTimeTimer(){
    struct itimerval timer;
    struct timeval timeTimer;
    timeTimer.tv_sec = 9;
    timeTimer.tv_usec = 0;
    struct timeval timeNextTimer;
    timeNextTimer.tv_sec = 0;
    timeNextTimer.tv_usec = 0;
    timer.it_interval = timeNextTimer;
    timer.it_value = timeTimer;
    if (setitimer(ITIMER_REAL, &timer, NULL)) {
        perror("setitimer");
        exit(EXIT_FAILURE);
    }
}

void allowAllChildSendStats(int signum){
    allowAllStats(childPidMas,childCount);
}

void setHandlerSIGALRM(){
    struct sigaction saSIGALRM;
    saSIGALRM.sa_handler = allowAllChildSendStats;
    saSIGALRM.sa_flags = SA_RESTART;
    if (sigaction(SIGALRM, &saSIGALRM, NULL) == -1) {
        perror("sigaction SIGALRM");
        exit(EXIT_FAILURE);
    }
}

void handlerSIGINT(int signum){
    printf("\nDESTROY CHILD AND PARENT\n");
    // kill(-(getpid()),SIGTERM);
    // while(wait(NULL) > 0);
    // fflush(stdout);
    // while(wait(NULL) > 0);

    destroyAllChildProcess(childPidMas,childCount);
    while(wait(NULL) > 0);
    char command[20];
    fflush(stdout);
    sprintf(command, "\npstree -p %d", getpid());
    system(command);
    exit(0);
}

void setHandlerSIGINT(){
    struct sigaction saSIGINT;
    saSIGINT.sa_handler = handlerSIGINT;
    if (sigaction(SIGINT, &saSIGINT, NULL) == -1) {
        perror("sigaction SIGINT");
        exit(EXIT_FAILURE);
    }
}

// void setHandlerSIGTERM(){
//     struct sigaction saSIGTERM;
//     saSIGTERM.sa_handler = handlerSIGINT;
//     if (sigaction(SIGINT, &saSIGTERM, NULL) == -1) {
//         perror("sigaction SIGALRM");
//         exit(EXIT_FAILURE);
//     }
// }

int main(int argc, char*argv[], char*envp[]) {
    setHandlerSIGINT();
    char symbols[20];
    int n;
    while (1){
        printf("\nEnter Symbol: ");
        fgets(symbols, 20, stdin);
        switch (symbols[0])
        {
        case '+':
            ++childCount;
            callChild(getenv("CHILD_PATH"), childPidMas, childCount);
            break;
        case '-':
            if (destroyChildProcess(childPidMas,childCount)){
                --childCount;
            }
            break;
        case 'l':
            listProcesses(childPidMas, childCount);
            break;
        case 'k':
            destroyAllChildProcess(childPidMas,childCount);
            childCount = -1;
            printf("\nAll child process destroy");
            break;
        case 'q':
            destroyAllChildProcess(childPidMas,childCount);
            printf("\nEXIT\n");
            while (wait(NULL) > 0);
            return 0;
        case 's':
            if (symbols[1] >= '0' && symbols[1] <= '9'){
                int n = atoi(symbols + 1);
                if (n <= childCount)
                    kill(childPidMas[n], SIGUSR2);
                else printf("\nThere is no child with this id = %d\n", n);
            }
            else banAllStats(childPidMas,childCount);
            break;
        case 'g':
            if (symbols[1] >= '0' && symbols[1] <= '9'){
                n = atoi(symbols + 1);
                if (n <= childCount)
                    kill(childPidMas[n], SIGUSR1);
                else printf("\nThere is no child with this id = %d\n", n);
            }
            else allowAllStats(childPidMas,childCount);
            break;
        case 'p':
            n = atoi(symbols + 1);
            if (n <= childCount && n >=0 ){
                banAllStats(childPidMas,childCount);
                setHandlerSIGALRM();
                setTimeTimer();
                kill(childPidMas[n], SIGUSR1);
            }
            else printf("\nThere is no child with this id = %d\n", n);
            break;
        }
    }
    while (wait(NULL) > 0);
    return 0;
}
