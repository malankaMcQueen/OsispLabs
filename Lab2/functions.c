#include <stdlib.h>
#include <string.h>
#include <stdio.h>

char* parsEnv(char** environ, const char* name){
    char** env = environ;
    while(*env != NULL){
        for(int i = 0; name[i] != '\0' && (*env)[i] != '=' && (*env)[i] != '\0'; i++){
            if (name[i] != (*env)[i])
                break;
            if (name[i + 1] == '\0' && (*env)[i+1] == '=')
                return ((*env) + i + 2 );
        }
        env++;
    }
    return NULL;
}

int compare_strings(const void *a, const void *b) {
    return strcmp(*(const char **)a, *(const char **)b);
}

char* addStrAndStr(char* str1, char* str2){
    if (str1 == NULL || str2 == NULL)
        return NULL;
    char* result = (char*)malloc(sizeof(char)* 500);
    strcpy(result, str1);
    strcat(result, str2);
    return result;
}

char* addStrAndNumb(char* str, int numb){
    if (str == NULL)
        return NULL;
    char* result = (char*)malloc(sizeof(char)* 9);
    strcpy(result, str);
    char buffer[5];
    snprintf(buffer,4,"%02d", numb);
    return strcat(result, buffer);
}