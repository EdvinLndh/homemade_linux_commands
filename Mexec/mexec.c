/*
  Author: c19elm, Edvin Lindholm
  Version: 1.1
  Date: sun  4 okt 2020


  Description: Program that functions as a shell pipeline.
*/
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <unistd.h>

#define WRITE_END 1
#define READ_END 0
#define BUFSIZE 1024

// Fuction declaration
void pipeline(char **array, int lineAmt);
void createPipes(int lineAmt, int file_desc[lineAmt -1][2]);
pid_t createFork(void);
void directPipes(int iteration, int lineAmt, int file_desc[lineAmt-1][2]);
void execute(char *command);


int main(int argc, char *argv[]) {
    FILE *inputfile;
    int lineAmt = 0;
    char buf[BUFSIZE];
    char *temp = NULL;
    // Array of commands.
    char **commands = malloc(sizeof(char *));
    if(commands == NULL) {
        perror("malloc");
        exit(EXIT_FAILURE);
    }

    // No inputfile, read from stdin.
    if(argc == 1) {
        inputfile = stdin;
    }
    // Read from inputfile.
    else if(argc == 2) {
        if ((inputfile = fopen(argv[1], "r")) == NULL) {
            perror(argv[1]);
            exit(EXIT_FAILURE);
        }
    }
    // Too many arguments, exit with error.
    else {
        fprintf(stderr, "Wrong number of arguments.\n");
        fprintf(stderr, "usage: %s file\n", argv[0]);
        exit(EXIT_FAILURE);
    }
    // Read file line by line.
    while(fgets(buf,BUFSIZE, inputfile) != NULL) {
        // Allocate a new space in array for each line input.
        commands = realloc(commands, (lineAmt + 1) * sizeof(char *));
        if(commands == NULL) {
            perror("realloc");
            exit(EXIT_FAILURE);
        }
        // Allocate space for line variable.
        temp =  malloc(strlen(buf) * sizeof(char));
        if(temp == NULL) {
            perror("malloc");
            exit(EXIT_FAILURE);
        }
        // Remove trailing \n and replace with \0.
        buf[strcspn(buf,"\n")] = '\0';
        strcpy(temp, buf);

        // Store in array.
        commands[lineAmt] = temp;
        // Count the amount of entries so far.
        ++lineAmt;
    }
    commands = realloc(commands, (lineAmt + 1) * sizeof(char *));
    if(commands == NULL) {
        perror("realloc");
        exit(EXIT_FAILURE);
    }
    commands[lineAmt] = 0;
    // Pipe and execute commands.
    pipeline(commands, lineAmt);

    // Free commands.
    for(int i = 0; i < lineAmt; i++) {
        free(commands[i]);
    }
    free(commands);

    fclose(inputfile);
    return 0;
}

/*  Function: pipeline
 *  Input:
            char **array    :Array to store commands.
            int lineAmt     :Amount of lines in file.

 *  Output: Executes the every command in array, by piping cmd1 -> cmd2 -> cmd3..., and so on, until last command
 *          is reached, in which output goes to terminal.
 */
void pipeline(char **array, int lineAmt) {

    // Create 2 file descriptors for each pipe. We will need one pipe for each line except the last.
    int file_desc[lineAmt - 1][2];
    pid_t pid;
    int status;
    int j;

    // Create pipes.
    createPipes(lineAmt, file_desc);


    // Create a process for each line, and pipe according to position of line.
    for(j = 0; j < lineAmt; j++) {
        pid = createFork();
        // Child processes:
        if(pid == 0) {
            directPipes(j, lineAmt, file_desc);
            // Leave loop after dup and close.
            break;
        }
    }

    // Parent process.
    if(pid > 0) {
        // Close all fd's.
        for(int i = 0; i < lineAmt-1; i++) {
            close(file_desc[i][READ_END]);
            close(file_desc[i][WRITE_END]);
        }
    }

    // Split commands in arguments and execute file.
    if(pid == 0) {
        execute(array[j]);

    }

    // Wait in children.
    if(pid > 0) {
        for(int i = 0; i < lineAmt; i++) {
            if (wait(&status) == -1) {
                perror("wait");
                exit(EXIT_FAILURE);
            }
            else if(WEXITSTATUS(status) != 0) {
                exit(EXIT_FAILURE);
            }
        }
    }
    return;
}

/*  Function: directPipes
 *  Input:
            int lineAmt     :Amount of lines in file.
            int file_desc   :The filedescriptors for the pipe, in a 2D array.
 *
 *  Output: Create pipes.
 */
void createPipes(int lineAmt, int file_desc[lineAmt -1][2]) {
    int ret;
    for(int i = 0; i < lineAmt-1; i++) {
        if((ret = pipe(file_desc[i])) == -1){
            perror("pipe");
            exit(EXIT_FAILURE);
        }
    }
    return;
}
/*  Function: createFork
 *  Input:
             void
 *
 *  Output: returns pid of child.
 */
pid_t createFork(void) {
    pid_t pid;
    if((pid = fork()) == -1) {
        perror("fork");
        exit(EXIT_FAILURE);
    }
    return pid;
}

/*  Function: directPipes
 *  Input:
            int iteration   :Current iteration loop.
            int lineAmt     :Amount of lines in file.
            int file_desc   :The filedescriptors for the pipe, in a 2D array.
 *
 *  Output: Uses dup2 and close to direct pipes.
 */
void directPipes(int iteration, int lineAmt, int file_desc[lineAmt-1][2]) {

    // First process:
    if(iteration == 0) {
        // Change output from STDOUT to pipe.
        if((dup2(file_desc[0][WRITE_END], STDOUT_FILENO))  == -1) {
            perror("dup2");
            exit(EXIT_FAILURE);
        }

        // Close all other pipe endings.
        for(int i = 0; i < lineAmt-1; i++) {
            if( i == 0) {
                close(file_desc[0][READ_END]);
            }
            close(file_desc[i][READ_END]);
            close(file_desc[i][WRITE_END]);
        }
    }
    // Last process:
    else if(iteration == lineAmt-1) {
        // Change input to come from previous pipes write end.
        if((dup2(file_desc[iteration-1][READ_END], STDIN_FILENO))  == -1) {
            perror("dup2");
            exit(EXIT_FAILURE);
        }

        // Close pipe endings.
        for(int i = 0; i < iteration; i++) {
            // Since we use read end of file descriptor iteration-1 we dont want to close it.
            if(i != iteration-1){
                close(file_desc[i][READ_END]);
                close(file_desc[i][WRITE_END]);
            }
            else {
                close(file_desc[iteration-1][WRITE_END]);
            }
        }
    }
    // Middle processes:
    else {
        // Change input to come from previous pipes write end.
        if(dup2(file_desc[iteration-1][READ_END], STDIN_FILENO) == -1) {
            perror("dup2");
            exit(EXIT_FAILURE);
        }
        // Change output to go to pipe.
        if(dup2(file_desc[iteration][WRITE_END], STDOUT_FILENO) == -1) {
            perror("dup2");
            exit(EXIT_FAILURE);
        }
        // Close pipe endings.
        for(int i = 0; i < iteration; i++) {
            // Since we use read end of file descriptor iteration-1 we dont want to close it.
            if(  i != iteration-1) {
                close(file_desc[i][READ_END]);
                close(file_desc[i][WRITE_END]);
            }
            else {
                close(file_desc[iteration-1][WRITE_END]);
            }
        }
        for(int i = iteration; i < lineAmt-1; i++) {
            // Since we use write end of file descriptor iteration we dont want to close it.
            if(i != iteration){
                close(file_desc[i][READ_END]);
                close(file_desc[i][WRITE_END]);
            }
            else {
                close(file_desc[iteration][READ_END]);
            }
        }
    }
}

/*  Function: execute
 *  Input:
            char *command   :Command to split into arguments and execute.
 *
 *  Output: Executes command and outputs it into pipe or if last line of file, to terminal.
 */
void execute(char *command) {
    int i = 0;
     // Array to store arguments in.
    char **argv = NULL;
    char *c;

    // Token c contains line without " ". (??)
    c = strtok(command, " ");
    if(c == NULL) {
         perror("strtok");
         exit(EXIT_FAILURE);
    }
    while (c) {
        // Allocate space for each argument.
        argv = realloc(argv, sizeof(char *) * (i + 1));
        if(argv == NULL) {
            perror("realloc");
            exit(EXIT_FAILURE);
        }
        // Store argument in array and continue until token is NULL.
        argv[i] = c;
        i++;
        c = strtok(NULL, " ");
    }
    // Allocate space for NULL.
    argv = realloc(argv, sizeof(char*) * (i + 1));
    if(argv == NULL) {
        perror("realloc");
        exit(EXIT_FAILURE);
    }
    // Add a NULL in the last position of the array.
    argv[i] = 0;

    // Execute programs.
    execvp(argv[0],argv);
    // Child shouldn't reach these lines.
    perror(argv[0]);
    exit(EXIT_FAILURE);


}
