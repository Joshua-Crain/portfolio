/* Name: Joshua Crain
 * Date: 11/1/2025
 *
 * CS 347 -- Mini Shell!
 * Original author Phil Nelson 2000
 */
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <ctype.h>
#include "argparse.h"
#include "builtin.h"


/* DEFINITIONS */

#define CMD_PROMPT "% "
#define MALLOC_ERR "error allocating space for argument"
#define WRITE_ERR "error on write call"
#define FGETS_ERR "error getting string from stream"
#define FORK_ERR "error creating child process for external program"
#define EXEC_ERR "error executing external program"
#define WAIT_ERR "error waiting for child process to terminate"
#define DEFAULT_BUFSIZE 20
#define BUFSIZE_INC 20

/* PROTOTYPES */

void processline(char *line);
ssize_t getinput(char** line, size_t* size);

/*
* main
*
* Main entry point of the myshell program.
* This is essentially the primary read-eval-print loop of the command interpreter.
*
* Runs the shell in an endless loop until an exit command is issued.
*
* Hint: Use getinput and processline as appropriate.
*/
int main() {
	size_t size = DEFAULT_BUFSIZE;
	char **line = calloc(1, sizeof(char **));

	while(1) { // program loop
		ssize_t line_len = getinput(line, &size);
		if (line_len == 0) { // no input, free and continue
			free(*line);
			continue;
		}

		processline(*line); // execute built-in function or external program

		free(*line);
	}

	return EXIT_SUCCESS;
}

/*
* getinput
*
* Prompts the user for a line of input (e.g. %myshell%) and stores it in a dynamically
* allocated buffer (pointed to by *line).
* If input fits in the buffer, it is stored in *line.
* If the buffer is too small, *line is freed and a larger buffer is allocated.
* The size of the buffer is stored in *size.
*
* Args:
*   line: pointer to a char* that will be set to the address of the input buffer
*   size: pointer to a size_t that will be set to the size of the buffer *line or 0 if *line is NULL.
*
* Returns:
*   The length of the the string stored in *line.
*
* Hint: There is a standard i/o function that can make getinput easier than it sounds.
*/
ssize_t getinput(char **line, size_t *size) {
	size_t buf_size = *size;
	ssize_t len = 0;
	char *buffer = malloc(buf_size * sizeof(char));
	char *temp_buffer;
	*line = buffer;
	
	if (write(STDOUT_FILENO, CMD_PROMPT, sizeof(CMD_PROMPT)) == -1) { // err check
		perror(WRITE_ERR);
		return 0;
	}
	
	// prompt for user input, and ensure all of it is taken. Resize buffer if necessary
	if (fgets(buffer, buf_size, stdin) == NULL) { // err check
		perror(FGETS_ERR);
		return 0;
	}

	while (buffer[strlen(buffer)-1] != '\n') { // a '\n' marks the end of user input. Resize by BUFSIZE_INC until argument fits.
		if ((temp_buffer = malloc(buf_size * sizeof(char))) == NULL) { // err check
			perror(MALLOC_ERR);
			return 0;
		}
		buf_size += BUFSIZE_INC;
		if ((buffer = malloc(buf_size * sizeof(char))) == NULL) { // err check
			perror(MALLOC_ERR);
			return 0;
		}

		strcpy(temp_buffer, *line); // old text goes to temp
		free(*line); // frees old buffer memory
		*line = buffer; // re-point line to buffer
		strcpy(buffer, temp_buffer); // copy old text back to buffer
		free(temp_buffer);

		if ((temp_buffer = malloc(buf_size * sizeof(char))) == NULL) { // err check
			perror(MALLOC_ERR);
			return 0;
		}
		// grab more of the argument
		if (fgets(temp_buffer, BUFSIZE_INC, stdin) == NULL) { // err check
			perror(FGETS_ERR);
			return 0;
		}

		strcat(buffer, temp_buffer);
		free(temp_buffer);
	}

	buffer[strlen(buffer) - 1] = '\0'; // replace ending '\n' with null terminator
	len = strlen(*line);
	return len;
}


/*
* processline
*
* Interprets the input line as a command and either executes it as a built-in
* or forks a child process to execute an external program.
* Built-in commands are executed immediately.
* External commands are parsed then forked to be executed.
*
* Args:
*   line: string containing a shell command and arguments
*
* Note: There are three cases to consider when forking a child process:
*   1. Fork fails
*   2. Fork succeeds and this is the child process
*   3. Fork succeeds and this is the parent process
*
* Hint: See the man page for fork(2) for more information.
* Hint: The process should only fork when the line is not empty and not trying to
*       run a built-in command.
*/
void processline(char *line)
{
	pid_t cpid;
	int status;
	int argCount;
	char** arguments = argparse(line, &argCount);

	if (arguments[0] == NULL) { // only white space, free and continue
		free(arguments);
		return;
	}

	if (!builtIn(arguments, argCount)) { // no built-in function? fork and execute externally
		cpid = fork();
		if (cpid > 0) { // parent
			wait(&status);
			if (status == -1) { // err check
				perror(WAIT_ERR);
			}
		}
		else if (cpid == 0) { // child
			status = execvp(arguments[0], arguments);
			if (status == -1) { // err check
				perror(EXEC_ERR);
			}
			exit(status);
		}
		else { // error
			perror(FORK_ERR);
		}
	}

	free(arguments);
}
