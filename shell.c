#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <mcheck.h>

#include "parser.h"
#include "shell.h"

/**
 * Program that simulates a simple shell.
 * The shell covers basic commands, including builtin commands 
 * (cd and exit only), standard I/O redirection and piping (|). 
 
 */

#define MAX_DIRNAME 100
#define MAX_COMMAND 1024
#define MAX_TOKEN 128

/* Functions to implement, see below after main */
int execute_cd(char** words);
int execute_nonbuiltin(simple_command *s);
int execute_simple_command(simple_command *cmd);
int execute_complex_command(command *cmd);


int main(int argc, char** argv) {
	
	char cwd[MAX_DIRNAME];           /* Current working directory */
	char command_line[MAX_COMMAND];  /* The command */
	char *tokens[MAX_TOKEN];         /* Command tokens (program name, 
					  * parameters, pipe, etc.) */

	while (1) {

		/* Display prompt */		
		getcwd(cwd, MAX_DIRNAME-1);
		printf("%s> ", cwd);
		
		/* Read the command line */
		fgets(command_line, MAX_COMMAND, stdin);
		/* Strip the new line character */
		if (command_line[strlen(command_line) - 1] == '\n') {
			command_line[strlen(command_line) - 1] = '\0';
		}
		
		/* Parse the command into tokens */
		parse_line(command_line, tokens);

		/* Check for empty command */
		if (!(*tokens)) {
			continue;
		}
		
		/* Construct chain of commands, if multiple commands */
		command *cmd = construct_command(tokens);
		//print_command(cmd, 0);
    
		int exitcode = 0;
		if (cmd->scmd) {
			exitcode = execute_simple_command(cmd->scmd);
			if (exitcode == -1) {
				break;
			}
		}
		else {
			exitcode = execute_complex_command(cmd);
			if (exitcode == -1) {
				break;
			}
		}
		release_command(cmd);
	}
    
	return 0;
}


/**
 * Changes directory to a path specified in the words argument;
 * For example: words[0] = "cd"
 *              words[1] = "csc209/assignment3/"
 * Your command should handle both relative paths to the current 
 * working directory, and absolute paths relative to root,
 * e.g., relative path:  cd csc209/assignment3/
 *       absolute path:  cd /u/bogdan/csc209/assignment3/
 */
int execute_cd(char** words) {
	
	/* make sure the tokens are not null or else this is not a correct token*/
    if(words == NULL){
        fprintf(stderr, "Error: the token is NULL\n");
        return EXIT_FAILURE;
    }
    if(strcmp(words[0], "cd")){
        fprintf(stderr, "Error: first token not cd\n");
        return EXIT_FAILURE;
    }
    if(words[1] == NULL){
        fprintf(stderr, "Error: second token is NULL\n");
        return EXIT_FAILURE;
    }

	 /*perform the directory move with chdir which accepts relative and absolute paths*/
	 int status = chdir(words[1]);
	 if(status == -1){
        perror(words[1]);
        return 1;
    }

	 return 0; /*dummy return */
	 
}


/**
 * Executes a program, based on the tokens provided as 
 * an argument.
 * For example, "ls -l" is represented in the tokens array by 
 * 2 strings "ls" and "-l", followed by a NULL token.
 * The command "ls -l | wc -l" will contain 5 tokens, 
 * followed by a NULL token. 
 */
int execute_command(char **tokens) {

	 /* execute tokens, and check errors*/
	int status = execvp(tokens[0], tokens);
    if(status == -1){
        perror(tokens[0]);
        return 1;
    }

    return 0; /* dummy return */


}


/**
 * Executes a non-builtin command.
 */
int execute_nonbuiltin(simple_command *s) {
	int fd;
	int status;

	/* processes of stdin, stdout, and stderr are similar, so create a forloop
	   to handle them all and arrays to tell the forloop what to do*/
	char* cmdstd[] = {s->in, s->out, s->err};
	FILE* std[] = {stdin, stdout, stderr};
	char *opening_err[] = {"opening stdin", "opening stdout", "opening stderr"};

	/*go through all stds and handle them*/
	int i;
	for (i=0; i < 3; i++) {
		if (cmdstd[i]) {
			/* stdin is needed just for reading while stdout and sterr
		       need to be written*/
			if (i==0) { 
				fd = open(s->in, O_RDONLY);
			}
			else {
				fd = open(cmdstd[i], O_WRONLY | O_CREAT | O_TRUNC, S_IRWXU);
			}
			/* check for opening error*/
	 		if (fd == -1) {
	 			perror(opening_err[i]);
	 			return 1;
	 		}		

			dup2(fd, fileno(std[i]));

			/*close and check if properly closed */
			status = close(fd);
		 	if (status == -1) {
		 		perror("fd closing");
	 			return 1;
		 	}		
	 	}
	}
	status = execute_command(s->tokens);
	return status;
}

/**
 * Executes a simple command (no pipes).
 */
int execute_simple_command(simple_command *cmd) {

	/* check if cd is called */
	if (cmd->builtin == BUILTIN_CD) {
		return execute_cd(cmd->tokens);
	}

	/* check if exit is called */
	else if (cmd->builtin == BUILTIN_EXIT) {
		return -1;
	}

	/*	non-builtin so fork a process to execute nonbuildin command. The 
		parent should wait for the child.*/
	else {
		pid_t pid = fork();
        if(pid == -1){
            perror("fork in simple command");
            return 1;
        }

        else if(pid == 0){
            exit(execute_nonbuiltin(cmd));
        }
		/* make parent wait for result of the child */
        else{
        	int status;
            if(wait(&status) == -1){
                perror("waiting in simple command");
            }
            return WEXITSTATUS(status);
        }
	}
}


/**
 * Executes a complex command.  A complex command is two commands chained 
 * together with a pipe operator.
 */
int execute_complex_command(command *c) {
	
	/* Check if this is a simple command using the scmd field, and ignore builtin commands*/
	if(c->scmd) {
		return execute_simple_command(c->scmd);
	}


	/** 
	 * Optional: if you wish to handle more than just the 
	 * pipe operator '|' (the '&&', ';' etc. operators), then 
	 * you can add more options here. 
	 */

	if (!strcmp(c->oper, "|")) {
		
		/**
		 * Create a pipe "pfd" that generates a pair of file 
		 * descriptors, to be used for communication between the 
		 * parent and the child. Error check.
		 */
		int pfd[2];


		/* moldy pipe */
		if(pipe(pfd) == -1) {
			perror("pipe complex command");
			return 1;
		}

		/* parent and child process are similar, so run a forloop, but switch the ends of the
		   pipe sides when reading in/out. do this by (i+1)%2. */
		pid_t children[2];
		int other_pfd;
		FILE* stdoutin[2] = {stdout, stdin};
		command* cmd[2] = {c->cmd1, c->cmd2};

		int i;
		for (i=0; i < 2; i++) {
			/*fork a new process*/
			children[i] = fork();

			/* error check the fork process*/
			if(children[i] == -1) {
				perror("child complex command");
				return 1;
			}

			if (children[i] == 0) {
				if (close(pfd[i]) == -1) {
					perror("close child complex command");
					exit(1);
				}
				/*connect to other side of the pipe*/
				other_pfd = pfd[(i+1)%2];
				if (dup2(other_pfd, fileno(stdoutin[i])) == -1) {
					perror("dup in complex command");
					exit(1);
				}

				if (close(other_pfd) == -1) {
					perror("close child complex command");
					exit(1);
				}
				/* fork a new process to execute the next command recursively*/
				exit(execute_complex_command(cmd[i]));
			}
		}
		/* close both ends of the pipe */
		if(close(pfd[0]) == -1) {
			perror("close left pipe");
			return 1;
		}
		if(close(pfd[1]) == -1) {
			perror("close right pipe");
			return 1;
		}
		/*wait for both children to finish*/
		pid_t pids[2];
		int pidstatus[2];
		pids[0] = wait(&pidstatus[0]);
		pids[1] = wait(&pidstatus[1]);

		if (pids[0] == -1|| pids[1] == -1) {
			perror("waiting");
			return 1;
		}

		if(WEXITSTATUS(pidstatus[0]) == 255 || WEXITSTATUS(pidstatus[1]) == 255) {
            return -1;
		}
		
	}
	
	return 0;
}
