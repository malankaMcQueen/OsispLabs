// #define _POSIX_C_SOURCE 199309L
// #define X_OPEN_SOURCE 700 
#define _POSIX_SOURCE 
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include "functions.c"
#include <sys/time.h>
#include <stdbool.h>

struct Pair {
    int first;
    int second;
};

struct Pair pair;
struct Pair const ZERO = {0,0};
struct Pair const ONE = {1,1};

int countTimes = 0;

int countZeroAndZero = 0;
int countOneAndOne = 0;
int countZeroAndOne = 0;
int countOneAndZero = 0;

bool permissionOutput = true;

void pairProcessing(int signum){
    if(pair.first == 0 && pair.second == 0)
        ++countZeroAndZero;
    else if(pair.first == 1 && pair.second == 1)
        ++countOneAndOne;
    else if(pair.first == 0 && pair.second == 1)
        ++countZeroAndOne;
    else if(pair.first == 1 && pair.second == 0)
        ++countOneAndZero;
    ++countTimes;
}

void setTimeTimer(){
    struct itimerval timer;
    struct timeval timeTimer;
    timeTimer.tv_sec = 0;
    timeTimer.tv_usec = 20001;
    timer.it_interval = timeTimer;
    timer.it_value = timeTimer;
    if (setitimer(ITIMER_REAL, &timer, NULL)) {
        perror("setitimer");
        exit(EXIT_FAILURE);
    }
}



void outputStatistics(char** argv){
    printf("\nSTATISTICS %s,PPID-%d, PID-%d : ZeroZero-%d, OneOne-%d, ZeroOne-%d, OneZero-%d\n",
            argv[0], getppid(), getpid(), countZeroAndZero, countOneAndOne, countZeroAndOne, countOneAndZero);
    countZeroAndZero = countOneAndOne = countZeroAndOne = countOneAndZero = 0;
    countTimes = 0;
}

void allowOutput(int signum){
    permissionOutput = true;
}

void banOutput(int signum){
    permissionOutput = false;
}

void setHandlerSIGALRM(){
    struct sigaction saSIGALRM;
    saSIGALRM.sa_handler = pairProcessing;
    saSIGALRM.sa_flags = SA_RESTART;
    if (sigaction(SIGALRM, &saSIGALRM, NULL) == -1) {
        perror("sigaction SIGALRM");
        exit(EXIT_FAILURE);
    }
}

void setHandlerSIGUSR1(){
    struct sigaction saSIGUSR1;
    saSIGUSR1.sa_handler = allowOutput;
    // saSIGUSR1.sa_flags= SA_RESTART;
    if (sigaction(SIGUSR1, &saSIGUSR1, NULL)) {
        perror("sigaction SIGUSR1");
        exit(EXIT_FAILURE);
    }
}

void setHandlerSIGUSR2(){
    struct sigaction saSIGUSR2;
    saSIGUSR2.sa_handler = banOutput;
    // saSIGUSR2.sa_flags= SA_RESTART;
    if (sigaction(SIGUSR2, &saSIGUSR2, NULL)) {
        perror("sigaction SIGUSR2");
        exit(EXIT_FAILURE);
    }
}

int main(int argc, char* argv[]){    
    setHandlerSIGALRM();
    setHandlerSIGUSR1();
    setHandlerSIGUSR2();
    setTimeTimer();
    while(1){
        pair = ZERO;
        pair = ONE;
        if (countTimes > 101 && permissionOutput == true){
            printf("\nSTATISTICS %s,PPID-%d, PID-%d : ZeroZero-%d, OneOne-%d, ZeroOne-%d, OneZero-%d\n",
            argv[0], getppid(), getpid(), countZeroAndZero, countOneAndOne, countZeroAndOne, countOneAndZero);
            countTimes = countZeroAndZero = countOneAndOne = countZeroAndOne = countOneAndZero = 0;
        }
    }
    exit(0);
}