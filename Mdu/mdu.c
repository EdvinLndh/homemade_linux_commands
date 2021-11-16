/*
*   Author: Edvin Lindholm (c19elm)
*   Date: fre  2 okt 2020
*   Version: 1.0
*
*   Description: A simple implementation of the du program. Gets the disk space for a given file.
*
*/

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>
#include <stdbool.h>
#include <getopt.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include "stack.h"
#include <pthread.h>


struct folders{
    stack *inptFiles;
    stack *subDirs;
    struct threadInfo *ti;
};

struct threadInfo {
    int sleepingThreads;
    int size;
    int threadNum;
};

int exit_code = 0;


pthread_mutex_t lock;
pthread_cond_t  cond;

void getArgs(int argc, char **argv, struct folders *folders);
int calcDir(char *path, stack *folders);
int calcFile(const char* fileName);
bool isFolder(char *filePath);
int recurseCalc(int size, DIR *dir, char *path, stack *subDirs);

// Thread function
void* worker(void *arg);

int main(int argc, char **argv) {
    // Init structs
    struct threadInfo *trdinfo = malloc(sizeof(struct threadInfo));
    if(trdinfo == NULL) {
        perror("malloc");
        exit(EXIT_FAILURE);
    }
    trdinfo->sleepingThreads = 0;
    trdinfo->size = 0;
    trdinfo->threadNum = 1;

    struct folders *folders = malloc(sizeof(struct folders));
    if(folders == NULL) {
        perror("malloc");
        exit(EXIT_FAILURE);
    }
    folders->inptFiles = stack_create();
    folders->subDirs = stack_create();
    folders->ti = trdinfo;

    // Get program arguments.
    getArgs(argc, argv, folders);

    // Add a folder so threads have something to work on.
    char* file = stack_top(folders->inptFiles);
    folders->subDirs = stack_push(folders->subDirs,file);

    //  Create threads.
    pthread_t thread[folders->ti->threadNum];
    for (int i = 0; i < folders->ti->threadNum; i++) {
        if(pthread_create(&thread[i], NULL, worker, (void *) folders) != 0) {
            fprintf(stderr, "Error creating thread.\n");
            exit(EXIT_FAILURE);
        }
    }

    // Wait in threads, when work is done.
    for (int i = 0; i < folders->ti->threadNum; i++) {
        if (pthread_join(thread[i], NULL) != 0) {
            fprintf(stderr, "Error joining thread.\n");
            exit(EXIT_FAILURE);
        }
    }

    // Deallocate and return.
    stack_kill(folders->inptFiles);
    stack_kill(folders->subDirs);

    free(trdinfo);
    free(folders);

    pthread_mutex_destroy(&lock);
    pthread_cond_destroy(&cond);

    return(exit_code);
}

/*  Function: getArgs
*   Input:
*             int argc      			:Argument counter from main.
* 			  int argv				    :Argument values from main.
*			  folders *folders 			:Variable struct storing options to update if options were input.
*
*  Output: Gets the list of files in the files struct, and also gets the amount of wanted threads, if specified.
*/
void getArgs(int argc, char **argv, struct folders *folders) {
    char* endp = NULL;
    int option;

    // Get options.
    while((option = getopt(argc, argv, "j:")) != -1) {
        switch(option) {
            case 'j':
            // Get numberr of threads.
            folders->ti->threadNum = (int) strtol(optarg, &endp, 10);
            break;

            default:
            printf("Wrong flag, options are: -j [threads] \n");
            exit(EXIT_FAILURE);
            break;
        }
    }

    // If more arguments are given
    // Push files into stack.
    for (int i = argc - optind; i > 0; i--) {
        // Allocate and copy into stack.
        char *str = malloc(sizeof(char*));
        if(str == NULL) {
            perror("malloc");
            exit(EXIT_FAILURE);
        }
        if(strcpy(str, argv[optind + i -1]) == NULL) {
            perror("strcpy");
            exit(EXIT_FAILURE);
        }
        folders->inptFiles = stack_push(folders->inptFiles, str);
    }
}


/*  Function: worker
*   Input:
*            void* arg
*
*  Output: Thread function, calculate sizes of the directories in stack.
*/
void *worker(void *arg) {

    struct folders *folders = (struct folders*)arg;
    while (true) {

        pthread_mutex_lock(&lock);
        //  Check if our stack with subfolders is empty ...
        if (!stack_empty(folders->subDirs)) {
            // ...if not, then work on folder.
            int size = calcDir(stack_pop(folders->subDirs),folders->subDirs);

            pthread_mutex_lock(&lock);
            // Add to size.
            folders->ti->size += size;
            pthread_mutex_unlock(&lock);

        } else {
            // ...if empty, go to sleep, unless there we're the last thread awake.
            folders->ti->sleepingThreads++;
            if(folders->ti->sleepingThreads == folders->ti->threadNum) {
                // Print the size of the input file.
                char *file = stack_pop(folders->inptFiles);
                printf("%d\t%s\n",folders->ti->size, file);
                free(file);
                // Reset size.
                folders->ti->size = 0;
                // If there's more work on the input file stack, push it to the other stack.
                if (!stack_empty(folders->inptFiles)) {

                    folders->subDirs = stack_push(folders->subDirs, stack_top(folders->inptFiles));

                    if (folders->ti->threadNum == 1) {
                        folders->ti->sleepingThreads--;
                        pthread_mutex_unlock(&lock);
                        continue;
                    }
                    // There's more work, wake a thread up.
                    pthread_cond_signal(&cond);
                } else {
                    // No more work, signal to all threads to return from function.
                    pthread_cond_broadcast(&cond);
                    pthread_mutex_unlock(&lock);
                    return arg;
                }
            }
            // Empty stack, go to sleep.
            while (true) {
                pthread_cond_wait(&cond,&lock);
                // Check if more work ...
                if (!stack_empty(folders->subDirs)) {
                    folders->ti->sleepingThreads--;
                    break;
                // If not return from function.
                }
                else if(folders->ti->sleepingThreads == folders->ti->threadNum) {
                    pthread_mutex_unlock(&lock);
                    return arg;
                }
            }
            pthread_mutex_unlock(&lock);
        }
    }
}

/*  Function: calcDir
*  Input:
*           const char *path:   Directory to iterate over.
*           stack *subFolders:  Stack of subfolders to iterate over.
*
*  Output:
*/
int calcDir(char *path, stack *subFolders) {
    pthread_mutex_unlock(&lock);
    int size = calcFile(path);

    // If the path is a file, return the size.
    if (!isFolder(path)) {
        free(path);
        return size;
    }
    // Otherwise it's a folder, add the size and open.
    int tot_size = size;
    DIR *dir = opendir(path);
    // If error, print errors, change exit code, but otherwise continue.
    if (dir == 0) {
        fprintf(stderr,"mdu: cannot read directory '%s':",path);
        free(path);
        pthread_mutex_lock(&lock);
        exit_code++;
        pthread_mutex_unlock(&lock);
        return size;
    }
    pthread_mutex_lock(&lock);
    // Recursively add sizes of files in folder.
    tot_size += recurseCalc(0,dir,path,subFolders);

    if (closedir(dir) != 0) {
        perror("closedir");
        exit(EXIT_FAILURE);
    }
    free(path);
    return tot_size;
}

/*  Function: recurseCalc
*  Input:
*           int size:           size of the content in directory.
*           DIR *dir:           Pointer to the opened directory.
*           char *path:         Path of the directory
*           stack *subFolders:  stack *subFolders:  Stack of subfolders to iterate over.
*
*  Output: The size of a the directory, including subdirectories.
*/
int recurseCalc(int size, DIR *dir, char *path, stack *subFolders) {

    pthread_mutex_unlock(&lock);
    struct dirent *entry;

    // If no more entries, return. Base for recursion.
    if ((entry = readdir(dir)) == NULL) {
        return size;
    }

    // Ignore "this directory" '.' and "previous directory" '..' entries.
    if (!((strcmp(entry->d_name, ".") == 0) || (strcmp(entry->d_name, "..") == 0))) {

        // Make a new filepath with the current path, and the entries name.
        char *newPath = malloc(sizeof(char)*(sizeof(path) + sizeof(entry->d_name) + 2) );
        if(newPath == NULL) {
            perror("malloc");
            exit(EXIT_FAILURE);
        }
        if (sprintf(newPath, "%s/%s", path, entry->d_name) < 0) {
            fprintf(stderr, "Problem concatenating string.\n");
            exit(EXIT_FAILURE);
        }
        if (isFolder(newPath)) {
            // if directory, push into stack.
            pthread_mutex_lock(&lock);
            stack_push(subFolders, newPath);
            // Signal that there's work.
            pthread_cond_signal(&cond);
            pthread_mutex_unlock(&lock);
        } else {
            // If it isn't a folder, it's probably a file.
            size += calcFile(newPath);
            free(newPath);
        }
    }
    pthread_mutex_lock(&lock);
    return recurseCalc(size,dir,path,subFolders);
}

/*  Function: calcFile
*   Input:   const char* filePath:  Filename to calculate size of.
*
*  Output: Size of directory, in blocks of 1024 B.
*/
int calcFile(const char* filePath){

    struct stat file_info;
    // Get info of file.
    if((lstat(filePath, &file_info)) == -1) {
        // If not, print error and exit.
        fprintf(stderr, "%s\n", filePath);
        perror("lstat");
        exit(EXIT_FAILURE);
    }
    // Return size.
    return file_info.st_blocks / 2;;
}

/*  Function: isFolder
*   Input:   const char* filePath:  Filename to check if folder or not.
*
*  Output: True if folder, otherwise false.
*/
bool isFolder(char *filePath) {
    struct stat st;
    // Get info of file
    if(lstat(filePath, &st) < 0) {
        perror("lstat");
        exit(EXIT_FAILURE);
    }
    // Return boolean.
    return S_ISDIR(st.st_mode);
}
