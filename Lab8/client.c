#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <time.h>

#define SERVER_IP "127.0.0.1"

void get_timestamp(char *buffer) {
    struct timespec ts;
    struct tm *tm;

    clock_gettime(CLOCK_REALTIME, &ts);
    tm = localtime(&ts.tv_sec);
    strftime(buffer, 24, "%Y.%m.%d-%H:%M:%S", tm);
    sprintf(buffer, "%s.%03ld", buffer, ts.tv_nsec / 1000000L);
}

void setConnction(int* serverSocket, int port) {
    struct sockaddr_in server_addr;
    // Create socket
    *serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (*serverSocket == -1) {
        perror("Failed to create socket");
        exit(EXIT_FAILURE);
    }
    // Set server address
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    if (inet_pton(AF_INET, SERVER_IP, &(server_addr.sin_addr)) <= 0) {
        perror("Invalid address/Address not supported");
        exit(EXIT_FAILURE);
    }
    // Connect to server
    if (connect(*serverSocket, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Connection failed");
        exit(EXIT_FAILURE);
    }
}

// void sendCommand
void sendCommandToServer(int serverSocket, char* command) {
    command[strlen(command) - 1] = '\0';
    strcat(command, " - ");
    char timeStamp[100];
    get_timestamp(timeStamp);
    strcat(command, timeStamp);
    if (send(serverSocket, command, 100, 0) == -1) {
        perror("Error sending command");
        close(serverSocket);
        exit(EXIT_FAILURE);
    }
}

void getResponseFromServer(int serverSocket) {
    char buffer[100];
    ssize_t bytesRead = read(serverSocket, buffer, 100);
    if (bytesRead == -1) {
        perror("Error receiving response.");
        close(serverSocket);
        exit(EXIT_FAILURE);
    }
    else if (bytesRead == 0) {
        perror("Server closed connection.");
        close(serverSocket);
        exit(EXIT_SUCCESS);
    }
    printf("%s\n", buffer);
}

void sendFromFile(int serverSocket, char* fileName) {
    printf("%s", fileName);
    fileName[strlen(fileName) - 1] = '\0';
    FILE* file = fopen(fileName, "r");
    if (file == NULL) {
        perror("Error opening file");
        return;
    }
    char buffer[100];
    while (fgets(buffer, 100, file) != NULL) {
        sendCommandToServer(serverSocket, buffer);
        getResponseFromServer(serverSocket);
    }
    fclose(file);

}

void getListDirectory(int serverSocket, char* command) {
    sendCommandToServer(serverSocket, command);
    char buffer[1024];
    while (1) {
        ssize_t bytesRead = read(serverSocket, buffer, sizeof(buffer));
        if (bytesRead == -1) {
            perror("Error receiving response.");
            close(serverSocket);
            exit(EXIT_FAILURE);
        }
        else if (bytesRead == 0) {
            perror("Server closed connection.");
            close(serverSocket);
            exit(EXIT_SUCCESS);
        }
        if (strncmp(buffer, "done", 4) == 0)
            break;
        printf("%s\n", buffer);
    }
}

void changeDirectory(int serverSocket, char* command, char* path) {
    sendCommandToServer(serverSocket, command);
    char buffer[100];
    ssize_t bytesRead = read(serverSocket, buffer, 100);
    if (bytesRead == -1) {
        perror("Error receiving response.");
        close(serverSocket);
        exit(EXIT_FAILURE);
    }
    else if (bytesRead == 0) {
        perror("Server closed connection.");
        close(serverSocket);
        exit(EXIT_SUCCESS);
    }
    strncpy(path, buffer + 18, strlen(buffer) - 18 - 26);
    path[strlen(buffer) - 18 - 26] = '\0';
    strcat(path, "> ");
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        fprintf(stderr, "ERROR, no port provided.");
        exit(1);
    }
    int port = atoi(argv[1]);
    int serverSocket;
    setConnction(&serverSocket, port);
    getResponseFromServer(serverSocket);
    int work = 1;
    char path[100];
    char command[100];
    strcpy(command, "cd .");
    changeDirectory(serverSocket, command, path);
    do {
        bzero(command, 100);
        printf("%s", path);
        fgets(command, 100, stdin);
        if (command[0] == '@'){
            sendFromFile(serverSocket, command + 1);
            continue;
        }
        if (strncmp(command, "quit", 4) == 0) {
            work = 0;
        }
        if (strncmp(command, "list", 4) == 0){
            getListDirectory(serverSocket, command);
            continue;
        }
        if (strncmp(command, "cd", 2) == 0) {
            changeDirectory(serverSocket, command, path);
            continue;
        }
        sendCommandToServer(serverSocket, command);
        getResponseFromServer(serverSocket);
    } while (work);

    close(serverSocket);

    return 0;
}