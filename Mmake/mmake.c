/*
*   Author: Edvin Lindholm (c19elm)
*   Date: fre  2 okt 2020
*   Version: 1.0
*
*   Description: A simple implementation of the make program. If no arguments are input, builds using "mmakefile".
*				 -f flag: Use other makefile than "mmakefile".
*				 -B flag: Force rebuild.
*				 -s flag: Mute output.
*				 Targets: mmake can take targets as input, and will build the input targets.
*
*/

#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <unistd.h>
#include "parser.h"
#include <sys/wait.h>
#include <string.h>
#include <errno.h>
#include <time.h>
#include <stdbool.h>


typedef struct {
	// Values for flags
	bool Bflag;
	bool sflag;
	// Index of targets.
	int targetIndex;

} optVariable;


// Function declaration.
char** getArgs(FILE **file, int argc, char **argv, optVariable *varp);
void recurseBuild(makefile *m, const char *target, optVariable var);
void build(rule *target_rule, optVariable var);

int main(int argc, char **argv) {

	FILE *fp = NULL;
	// Defualt option values.
	optVariable var = {false, false, 0};
	optVariable *varp = &var;

	char **targetList = NULL;

	// Get list of targets and update flag status.
	targetList = getArgs(&fp, argc, argv, varp);

	// If no flag or argument were input, default value to open is "mmakefile".
	if(fp == NULL) {
		fp = fopen("mmakefile", "r");
		if (fp == NULL) {
			perror("mmakefile");
			exit(EXIT_FAILURE);
		}
	}

	// Parse makefile.
	makefile *m = parse_makefile(fp);
	if (m == NULL) {
		fprintf(stderr, "mmakefile: Could not parse makefile\n");
		exit(EXIT_FAILURE);
	}

	// Close file
	fclose(fp);

	// Get makefiles default target.
	const char *defTarget = makefile_default_target(m);

	// If there wasn't any individual targets, we want to build using a makefile.
	if(targetList == NULL){
		recurseBuild(m, defTarget, var);
	}
	// Else, build using the targets.
	else {
		for(int i = 0; i < (argc-var.targetIndex); ++i) {
			rule *rule = makefile_rule(m, targetList[i]);
			if (rule == NULL) {
				printf("%s: Rule not found\n", targetList[i]);
				continue;
			}
			build(rule, var);
		}
	}
	// Free memory.
	makefile_del(m);
	free(targetList);
	return 0;
}

/*  Function: getArgs
*  Input:
*            file **file    			:File to update.
*            int argc      			:Argument count from main.
* 			  int argv				    :Argument values from main.
*			  optVaribale varp 			:Variable struct storing options to update if options were input.
*
*  Output: If there are targets build, getArgs stores these in an array. Also updates options.
*  		getArgs uses getopt to get index of targets and change flag status.
*/
char** getArgs(FILE **file, int argc, char **argv, optVariable *varp) {
	char **targetList = NULL;
	int option;
	// Get options and accompanying arguments.
	while((option = getopt(argc, argv, "f:Bs")) != -1) {
		switch (option) {
			// F flag is used to use other targets instead of makefile.
			case 'f':
			// optarg to get arguments.
			*file = fopen(optarg, "r");
			if(file == NULL) {
				perror(optarg);
				exit(EXIT_FAILURE);
			}
			break;
			// B Flag Force rebuild.
			case 'B':
			varp->Bflag = true;
			break;
			// s flag: Supress output on terminal.
			case 's':
			varp->sflag = true;
			break;
			// Wrong option, print error.
			default:
			printf("opt: %c\n", option);
			printf("optarg: %s\n", optarg);
			break;
		}
	}

	// If targets were input, store these in a list using optind to get index of said targets.
	if (argv[optind] != NULL) {
		// Allocate array.
		targetList = malloc(sizeof(char *) * ((argc-optind) + 1));
		if(targetList == NULL) {
			perror("malloc");
			exit(EXIT_FAILURE);
		}
		// Store targets in array.
		for (int i = 0; i < (argc - optind); ++i) {
			targetList[i] = argv[optind + i];
		}
		// NULL terminate array.
		targetList[(argc - optind)] = NULL;
	}
	varp->targetIndex = optind;
	return targetList;
}


/*  Function: recurseBuild
*  Input:
makefile m    			:Parsed makefile
const char *target      :Target to check prerequisites of and eventually build.
optVariable var			:Current flag statuses.
*
*  Output: Recrusively builds the rules in the makefile
*/
void recurseBuild(makefile *m, const char *target, optVariable var) {

	int index = 0;
	struct stat prereqInfo;
	struct stat targetInfo;

	rule *target_rule = makefile_rule(m, target);
	// No rule exists, so we continue.
	if(target_rule == NULL) {
		// Check that target exists.
		if(stat(target, &targetInfo) == -1) {
			perror(target);
			exit(EXIT_FAILURE);
		}
		return;
	}
	// Get list of prerequisites.
	const char **prereqList = rule_prereq(target_rule);

	// Iterate down the list of prerequisites, to build from the bottom and up.
	while(prereqList[index] != NULL) {
		recurseBuild(m, prereqList[index], var);
		++index;
	}
	// If the target doesn't exist, we want to build.
	if(stat(target,&targetInfo) == -1) {
		build(target_rule, var);
		return;
	}

	index = 0;

	//Loop prerequisites and recursively build file.
	while(prereqList[index] != NULL) {

		// Get info about the time of last modification. Prerequiste has to exist at this point.
		if (stat(prereqList[index], &prereqInfo) == -1) {
			perror(prereqList[index]);
			exit(EXIT_FAILURE);
		}

		// If B-flag is true, we want to force rebuild, if the time of last modiciation of the prerequiste is sooner than
		// targets last modification, and that time is more than one second we also want to build.
		if(var.Bflag == true || (difftime(prereqInfo.st_mtime, targetInfo.st_mtime) > 1)) {
			build(target_rule, var);
			break;
		}
		++index;
	}
	return;
}

/*  Function: Build
*  Input:
*          rule *target_rule      :Rule to build,
*			optVariable var		   :Current flag statuses.
*
*  Output: Builds target rule, outputs command built.
*/
void build(rule *target_rule, optVariable var) {

	pid_t pid;
	int status;
	int i = 0;

	// Get commandline to execute.
	char** cmd = rule_cmd(target_rule);
	// Make child
	if((pid = fork()) == -1) {
		perror("fork");
		exit(EXIT_FAILURE);
	}
	// Child executes commands.
	if(pid == 0) {
		execvp(cmd[0], cmd);
		// Child shouldn't reach these lines.
		perror(cmd[0]);
		exit(EXIT_FAILURE);
	}
	else {
		// If s-flag is true we don't want to write.
		if(var.sflag != true) {
			// Otherwise write command and arguments, add spaces between arguments and newline when command is done.
			while(cmd[i] != NULL) {
				printf("%s", cmd[i]);
				++i;
				if(cmd[i] != NULL) {
					printf(" ");
				}
				else{
					printf("\n");
				}
			}
		}
		// Wait for children.
		if (wait(&status) == -1) {
			perror("wait");
			exit(EXIT_FAILURE);
		}
		else if(WEXITSTATUS(status) != 0) {
			exit(EXIT_FAILURE);
		}
	}

}
