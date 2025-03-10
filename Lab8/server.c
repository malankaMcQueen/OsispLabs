#include <strings.h>
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <dirent.h>
#include <sys/stat.h>
#include <pthread.h>


enum Command {
    QUIT = 0,
    INFO,
    ECHO,
    CD,
    LIST,
};

void get_timestamp(char *buffer) {
    struct timespec ts;
    struct tm *tm;

    clock_gettime(CLOCK_REALTIME, &ts);
    tm = localtime(&ts.tv_sec);
    strftime(buffer, 24, "%Y.%m.%d-%H:%M:%S", tm);
    sprintf(buffer, "%s.%03ld", buffer, ts.tv_nsec / 1000000L);
}


void setConnection(int* serverSocket, int port) {
    struct sockaddr_in serverAddr;
    *serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (*serverSocket < 0) {
        perror("ERROR opening socket\n");
        exit(1);
    }

    int optVal = 1;
    if (setsockopt(*serverSocket, SOL_SOCKET, SO_REUSEADDR, &optVal, sizeof(optVal)) < 0) {
        perror("Error: Could not set SO_REUSEADDR option\n");
    }

    bzero((char *) &serverAddr, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(port);

    if (bind(*serverSocket, (struct sockaddr *) &serverAddr, sizeof(serverAddr)) < 0) {
        perror("ERROR on binding");
        exit(1);
    }

    listen(*serverSocket, 5);
}

void sendMessage(int clientSocked, char* command) {  
    // get_timestamp(command + strlen(command));
    strcat(command, " - ");
    char timestamp[100];
    get_timestamp(timestamp);
    strcat(command, timestamp);
    if (send(clientSocked, command, 100, 0) == -1) {
        perror("Error sending command");
        close(clientSocked);
        exit(EXIT_FAILURE);
    }
}

void sendInfoAboutServer(int clientSocket){
    int file = open("/home/bobr/OsispLabs/Lab8/info.txt", O_RDONLY);
    if (file == -1) {
        perror("open");
        sendMessage(clientSocket, "Error open info file");
        return;
    }
    char buffer[100];
    ssize_t bytesRead;
    bytesRead = read(file, buffer, sizeof(buffer) - 1);
    buffer[bytesRead] = '\0';
    close(file);
    sendMessage(clientSocket, buffer);
}

enum Command parseCommand(int clientSocket, char* command) {
    if (strncmp(command, "quit", 4) == 0) {
        return QUIT;
    }
    if (strncmp(command, "info", 4) == 0) {
        return INFO;
    }
    if (strncmp(command, "echo", 4) == 0) {
        return ECHO;
    }
    if (strncmp(command, "cd", 2) == 0) {
        return CD;
    }
    if (strncmp(command, "list", 4) == 0) {
        return LIST;
    }
    return -1;
}

void waitMessage(int clientSocket, char* command) {
    ssize_t bytesRead = read(clientSocket, command, 100);
    if (bytesRead == -1) {
        perror("Error receiving response.");
        close(clientSocket);
        exit(EXIT_FAILURE);
    }
    else if (bytesRead == 0) {
        perror("Server closed connection.");
        close(clientSocket);
        exit(EXIT_SUCCESS);
    }
}

void sendDirectoryListing(int clientSocket, const char *dirPath) {
    DIR *dir = opendir(dirPath);
    if (dir == NULL) {
        perror("opendir");
        sendMessage(clientSocket, "Error opening directory");
        return;
    }
    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        struct stat entryStat;
        char fullPath[1024];
        snprintf(fullPath, sizeof(fullPath), "%s/%s", dirPath, entry->d_name);
        if (lstat(fullPath, &entryStat) == -1) {
            perror("lstat");
            continue;
        }
        char buffer[1024];
        if (S_ISDIR(entryStat.st_mode)) {
            snprintf(buffer, sizeof(buffer), "/%s/", entry->d_name);
        } else if (S_ISLNK(entryStat.st_mode)) {
            char target[1024];
            ssize_t len = readlink(fullPath, target, sizeof(target)-1);
            if (len != -1) {
                target[len] = '\0';
                snprintf(buffer, sizeof(buffer), "%s --> %s", entry->d_name, target);
            } else {
                perror("readlink");
                continue;
            }
        } else {
            snprintf(buffer, sizeof(buffer), "%s", entry->d_name);
        }
        if (send(clientSocket, buffer, sizeof(buffer), 0) == -1) {
            perror("Error sending command");
            close(clientSocket);
            exit(EXIT_FAILURE);
        }
    }
    closedir(dir);
    if (send(clientSocket, "done", 100, 0) == -1) {
        perror("Error sending command");
        close(clientSocket);
        exit(EXIT_FAILURE);
    }
}

void changeDirectory(int clientSocket, char* command, char* dirPath) {
    char newDir[1024];
    bzero(newDir,sizeof(newDir));
    char tempPath[1024];
    bzero(tempPath,sizeof(tempPath));
    strncpy(newDir, command + 3, strlen(command) - 3 - 26); // Extract directory from command
    newDir[strlen(newDir)] = '\0'; // Remove newline character
    if (newDir[0] == '/') {
        sendMessage(clientSocket, "Error: Absolute path not allowed");
        return;
    }
    else if (strncmp(newDir, "..", 2) == 0){
        strcpy(tempPath, dirPath);
        for (int i = strlen(dirPath) - 1; i >= 0; i--) {
            if (tempPath[i] == '/') {
                tempPath[i] = '\0';
                break;
            }
        }
    }
    else if (strncmp(newDir, ".", 1) == 0){
        strcpy(tempPath, dirPath);
    }
    else {
        strcpy(tempPath, dirPath);
        strcat(tempPath, "/");
        strcat(tempPath, newDir);
    }
    char buffer[100];
    if(chdir(tempPath) == 0) { // Change directory
        strcpy(dirPath, tempPath); 
        strcpy(buffer, "Current directory: ");
        strcat(buffer, dirPath);
        sendMessage(clientSocket, buffer);
    } else {
        perror("chdir");
        strcpy(buffer, "Current directory: ");
        strcat(buffer, dirPath);
        sendMessage(clientSocket, buffer);
    }
}

void waitConnection(int newClientSocked, char* dirPath) {
    
    printf("New client connected\n");
    sendInfoAboutServer(newClientSocked);
    int clientConnect = 1;
    char command[100];
    char echoBuf[100];
    while (clientConnect){
        waitMessage(newClientSocked, command);
        printf("%s\n", command);
        fflush(stdout);
        switch (parseCommand(newClientSocked, command)) {
            case INFO:
                sendInfoAboutServer(newClientSocked);
                break;
            case QUIT:
                strcpy(echoBuf, "BYE");
                sendMessage(newClientSocked, echoBuf);
                close(newClientSocked);
                clientConnect = 0;
                break;
            case ECHO:
                strncpy(echoBuf, command + 5, strlen(command) - 5 - 26);
                sendMessage(newClientSocked, echoBuf);
                break;
            case CD:
                changeDirectory(newClientSocked, command, dirPath);
                break;
            case LIST:
                sendDirectoryListing(newClientSocked, dirPath);
                break;
            default:
                strcpy(echoBuf, "Unknown command");
                sendMessage(newClientSocked, echoBuf);
        }
        bzero(echoBuf, 100);

    }
    close(newClientSocked);
}
struct ThreadArgs {
    int clientSocket;
    char* dirPath;
};


void *handleConnection(void *args) {
    struct ThreadArgs *threadArgs = (struct ThreadArgs *)args;
    int newClientSocked = threadArgs->clientSocket;
    char dirPath[100];
    strcpy(dirPath, threadArgs->dirPath);
    waitConnection(newClientSocked, dirPath);
    return NULL;
}


int main(int argc, char* argv[]) {
    if (argc < 3) {
        fprintf(stderr, "ERROR, PORT + PATH\n");
        exit(1);
    }
    int port = atoi(argv[1]);
    int serverSocket;
    char startDir[100];
    strcpy(startDir, argv[2]);
    setConnection(&serverSocket, port);
    while (1) {
        struct sockaddr_in clientAddr;
        socklen_t addrLen = sizeof(clientAddr);
        int newClientSocked = accept(serverSocket, (struct sockaddr *) &clientAddr, &addrLen);
        if (newClientSocked < 0) {
            perror("ERROR on accept");
            exit(1);
        }
        struct ThreadArgs *args = malloc(sizeof(struct ThreadArgs));
        args->clientSocket = newClientSocked;
        args->dirPath = startDir;
        pthread_t threadId;
        if (pthread_create(&threadId, NULL, handleConnection, (void *)args) != 0) {
            perror("ERROR on pthread_create");
            exit(1);
        }
        pthread_detach(threadId);
    }
    waitConnection(serverSocket, startDir);

    close(serverSocket);
    return 0;
}