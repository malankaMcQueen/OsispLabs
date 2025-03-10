#include <sys/types.h>
#include <dirent.h>
#include <stdlib.h>
#include <stdio.h>
#include <limits.h>
#include <unistd.h>

#include <getopt.h>
#include <stdbool.h>
#include <errno.h>
#include <dirent.h>
#include <string.h>
#include <sys/stat.h>
#include <linux/stat.h>
#include <sys/sysmacros.h>
#include <locale.h>

int locale_strcmp(const struct dirent **a, const struct dirent **b) {
    return strcoll((*a)->d_name, (*b)->d_name);
}


void dirwalkStdout(char *path, bool l_opt, bool d_opt, bool f_opt,bool s_opt){

    struct dirent* dirEntry;
    struct stat sb;
    char fullpath[256];
    struct dirent **namelist;
 
    int numb = scandir(path,&namelist,NULL,s_opt ? locale_strcmp: NULL);
    if (numb<=0){
        perror("scandir");
        return;
    }

    for(size_t k = 0; k < numb; k++){
        dirEntry = namelist[k];
        if ((strcmp(dirEntry->d_name,".") == 0 ) || (strcmp(dirEntry->d_name,"..") == 0 )){
            free(namelist[k]);
            continue;
        }
        memset(fullpath, '\0', 256);        
        snprintf(fullpath, sizeof(fullpath), "%s/%s", path, dirEntry->d_name);

        if (lstat(fullpath,&sb) == -1){
            perror("lstat error");
            strerror(errno);
            continue;
        }
        switch(__S_IFMT & sb.st_mode) {
            case __S_IFDIR:
                if (d_opt)
                    printf("\n DIR: %s",fullpath);
                dirwalkStdout(fullpath,l_opt,d_opt,f_opt,s_opt);
                break;
            case __S_IFLNK:
                if(l_opt)
                    printf("\nLNK: %s",fullpath);
                break;
            case __S_IFREG:
                if(f_opt)
                    printf("\nFILE: %s",fullpath);
                break;
            default: break;
        }
        free(namelist[k]);   
    }
}


int main(int argc,char** argv){
    bool l_opt = false;
    bool d_opt = false;
    bool f_opt = false;
    bool s_opt = false;
    int opt;

    struct option long_options[] = {
        {"l", no_argument, 0, 'l'},
        {"d", no_argument, 0, 'd'},
        {"f", no_argument, 0, 'f'},
        {"s", no_argument, 0, 's'},
        {0, 0, 0, 0}
    };

    while ((opt = getopt_long(argc, argv, "ldfs", long_options, NULL)) != -1) {
        switch (opt) {
            case 'l':
                printf("l\n");
                l_opt = true;
                break;
            case 'd':
                printf("d\n");
                d_opt = true;
                break;
            case 'f':
                printf("f\n");
                f_opt = true;
                break;
            case 's':
                printf("s\n");
                s_opt = true;
                break;
            default:
                return 0;
                break;
        }
    }
    if (!(l_opt || d_opt || f_opt)){
        l_opt = true;
        d_opt = true;
        f_opt = true;
    }
    bool haveName = false;
    char* path;

    for (int i = 1; i < argc ; i++){
        if(argv[i][0] != '-'){
            if (haveName == true){
                printf("incorrect syntax");
                return 0;
            }
            else {
                haveName = true;
                path = argv[i];
            }
        }
    }
    
    if (haveName)
        dirwalkStdout(path,l_opt,d_opt,f_opt,s_opt);
    else
    хуй
        dirwalkStdout("./",l_opt,d_opt,f_opt,s_opt);
    return 0;
}    
