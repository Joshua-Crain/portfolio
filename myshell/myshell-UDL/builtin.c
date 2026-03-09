/* Name: Joshua Crain
 * Date: 11/1/2025
 */
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <ctype.h>
#include "builtin.h"
#include <dirent.h>
#include <time.h>
#include <grp.h>
#include <pwd.h>
#include <sys/sysmacros.h>
#include <utime.h>

//Definitions
#define DEFAULT_BUFSIZE 20
#define BUFSIZE_INC 20
#define STAT_USAGE "stat: missing operand\n"
#define TAIL_USAGE "tail: missing file operand\n"
#define TOUCH_USAGE "touch: missing file operand\n"
#define STAT_ERR "error calling stat"
#define PWD_ERR "error getting current directory"
#define CD_ERR "error changing directory"
#define MALLOC_ERR "error allocating space for argument"
#define FGETS_ERR "error getting string from stream"
#define OPEN_ERR "error opening file"
#define LSEEK_ERR "error calling lseek"
#define READ_ERR "error calling read"
#define WRITE_ERR "error calling write"
#define CLOSE_ERR "error calling close"
#define TIME_ERR "error getting the current system time"

//Prototypes
static void exitProgram(char** args, int argcp);
static void cd(char** args, int argpcp);
static void pwd(char** args, int argcp);

//Group A
static void cmdA_ls(char** args, int argcp);
static void cmdA_cp(char** args, int argcp);
static void cmdA_env(char** args, int argcp);

//Group B
static void cmdB_stat(char** args, int argcp);
static void cmdB_tail(char** args, int argcp);
static void cmdB_touch(char** args, int argcp);

//Helpers
void get_perms(char *buf, mode_t mode);
void get_perms_oct(int *buf, mode_t mode);
void d_or_f(char *buf, mode_t mode);

/*
* builtIn
*
* Checks if a command is a built-in shell command and executes it if so.
*
* Args:
*   args: array of strings containing command and arguments
*   argcp: number of elements in args array
*
* Returns:
*   1 if the command was a built-in, 0 otherwise.
*
* Built-in commands are executed directly by the shell process rather than
* being forked to a new process. This function compares the given command
* to each of the built-ins (exit, pwd, cd, and ls/cp/env or stat/tail/touch
* depending on group). If a match is found, the corresponding function is called.
*
* Hint: Refer to checklist for group specific examples
*/
int builtIn(char** args, int argcp)
{
	// me when robust code
    if (strcmp(args[0], "exit") == 0) {
		exitProgram(args, argcp);
	}
	else if (strcmp(args[0], "pwd") == 0) {
		pwd(args, argcp);
	}
	else if (strcmp(args[0], "cd") == 0) {
		cd(args, argcp);
	}
	else if (strcmp(args[0], "stat") == 0) {
		cmdB_stat(args, argcp);
	}
	else if (strcmp(args[0], "tail") == 0) {
		cmdB_tail(args, argcp);
	}
	else if (strcmp(args[0], "touch") == 0) {
		cmdB_touch(args, argcp);
	}
	else { // fail
		return 0;
	}
	// success
	return 1;
}

/*
* exitProgram
*
* Terminates the shell with a given exit status.
* If no exit status is provided, exits with status 0.
* This function should use the exit(3) library call.
*
* Args:
*   args: array of strings containing "exit" and optionally an exit status
*   argcp: number of elements in args array
*/
static void exitProgram(char** args, int argcp) {
	if (argcp > 1) { // exit status given?
		exit(atoi(args[1]));
	}
	else { // no status, default 0
		exit(0);
	}
}

/*
* pwd
*
* Prints the current working directory.
*
* Args:
*   args: array of strings containing "pwd"
*   argcp: number of elements in args array, should be 1
*
* Example Usage:
*   Command: $ pwd
*   Output: /some/path/to/directory
*/
static void pwd(char** args, int argcp) {
	size_t buf_size = DEFAULT_BUFSIZE;
	char *buf;
	if ((buf = malloc(buf_size * sizeof(char))) == NULL) { // err check
			perror(MALLOC_ERR);
    		errno = 0;
			return;
	}

	getcwd(buf, buf_size);

	while(errno == ERANGE) { // buffer too small? resize until it fits
		errno = 0;
		buf_size += BUFSIZE_INC;
		free(buf);

		if ((buf = malloc(buf_size * sizeof(char))) == NULL) { // err check
			perror(MALLOC_ERR);
    		errno = 0;
			return;
		}

		getcwd(buf, buf_size);
	}

	printf("%s\n", buf);
	free(buf);
}

/*
* cd
*
* Changes the current working directory.
* When no parameters are provided, changes to the home directory.
* Supports . (current directory) and .. (parent directory).
*
* Args:
*   args: array of strings containing "cd" and optionally a directory path
*   argcp: number of elements in args array
*
* Example Usage:
*   Command: $ pwd
*   Output: /some/path/to/directory
*   Command: $ cd ..
*   Command: $ pwd
*   Output: /some/path/to
*
* Hint: Read the man page for chdir(2)
*/
static void cd(char** args, int argcp) {
	if (argcp > 1) { // path given
		if (chdir(args[1]) < 0) { // err check
			perror(CD_ERR);
    		errno = 0;
		}	
	}
	else { // no path given, go to home directory
		char *home_dir = getenv("HOME");
		if (chdir(home_dir) < 0) { // err check
			perror(CD_ERR);
    		errno = 0;
		}	
	}
}

/*
* stat
*
* Prints information about the given files by accessing the associated file data from the 
* stat() function.
* 
* Args:
* args: array of strings containing "stat" and the file or directory names
* argcp: number of elements in args array
*/
static void cmdB_stat(char **args, int argcp) {
	if (argcp < 2) { // use case check
		fprintf(stderr, STAT_USAGE);
    	errno = 0;
		return;
	}

	FILE *file;
	struct stat *stat_struct;
	struct passwd *passwd_struct;
	struct group *group_struct;
	char *dir_or_fil;
	char *perms;
	int *perms_oct;
	char *user;
	char *group;

	for (int i = 1; i < argcp; i++) { // iterate through given files/paths and prints them
		if ((stat_struct = malloc(sizeof(struct stat))) == NULL) { // err check
			fprintf(stderr, "\"%s\": ", args[i]);
			perror(MALLOC_ERR);
			errno = 0;
			return;
		}

		// get struct stat for file
		if(stat(args[i], stat_struct) < 0) { // error check
			free(stat_struct);
			fprintf(stderr, "\"%s\": ", args[i]);
			perror(STAT_ERR);
    		errno = 0;
			continue;
		}
		
		// get label for regular file or directory
		if ((dir_or_fil = calloc(DEFAULT_BUFSIZE, sizeof(char))) == NULL) { // err check
			perror(MALLOC_ERR);
			errno = 0;
			return;
		}

		d_or_f(dir_or_fil, stat_struct->st_mode);

		// get file/directory and read, write, execute perm values for the drwxrwxrwx format
		if ((perms = calloc(11, sizeof(char))) == NULL) { // 10 chars in drwxrwxrwx and err check
			fprintf(stderr, "\"%s\": ", args[i]);
			perror(MALLOC_ERR);
			errno = 0;
			return;
		}

		if ((perms_oct = calloc(3, sizeof(int))) == NULL) { // 3 octal digits and err check
			fprintf(stderr, "\"%s\": ", args[i]);
			perror(MALLOC_ERR);
			errno = 0;
			return;
		}

		get_perms(perms, stat_struct->st_mode);
		get_perms_oct(perms_oct, stat_struct->st_mode);

		// get user name and group name
		passwd_struct = getpwuid(stat_struct->st_uid);
		group_struct = getgrgid(stat_struct->st_gid);
		user = passwd_struct->pw_name;
		group = group_struct->gr_name;

		// print output in proper format
		printf("File: %s\n", args[i]);
		printf("Size: %d\t Blocks: %d\t IO Block: %d\t %s\n", 
			stat_struct->st_size, stat_struct->st_blocks, stat_struct->st_blksize, dir_or_fil);
		printf("Device: %d,%d\t Inode: %d\t Links: %d\n", 
			major(stat_struct->st_dev), minor(stat_struct->st_dev), stat_struct->st_ino, stat_struct->st_nlink);
		printf("Permissions: (%o%o%o/%s)\t UID: (%d, %s)\t GID: (%d, %s)\n", 
			perms_oct[0], perms_oct[1], perms_oct[2], perms, stat_struct->st_uid, user, stat_struct->st_gid, group);
		printf("Access: %s", ctime(&stat_struct->st_atime));
		printf("Modify: %s", ctime(&stat_struct->st_mtime));
		printf("Change: %s", ctime(&stat_struct->st_ctime));
		printf("\n");

		// free space
		free(dir_or_fil);
		free(stat_struct);
		free(perms);
		free(perms_oct);
	}
}
/*
* get_perms
*
* Returns string in format of drwxrwxrwx based on given file permissions.
*
* Args:
* buf: Buffer for output
* mode: A st_mode value for a file
*/
void get_perms(char *buf, mode_t mode) {
	// initialize as ----------
	for(int i = 0; i < 10; i++) {
		buf[i] = '-';
	}
	buf[10] = '\0';
	
	// assign perm letters where applicable
	if (S_ISDIR(mode)) buf[0] = 'd'; 
	if (mode & S_IRUSR) buf[1] = 'r'; 
	if (mode & S_IWUSR) buf[2] = 'w'; 
	if (mode & S_IXUSR) buf[3] = 'x'; 
	if (mode & S_IRGRP) buf[4] = 'r'; 
	if (mode & S_IWGRP) buf[5] = 'w'; 
	if (mode & S_IXGRP) buf[6] = 'x'; 
	if (mode & S_IROTH) buf[7] = 'r'; 
	if (mode & S_IWOTH) buf[8] = 'w'; 
	if (mode & S_IXOTH) buf[9] = 'x'; 
}

/*
* get_perms_oct
*
* Returns string in format of xxx based on given file permissions,
* where "x" is the user, group, other permission digits in octal.
*
* Args:
* buf: Buffer for output
* mode: A st_mode value for a file
*/
void get_perms_oct(int *buf, mode_t mode) {
	
	// get perms
	if (mode & S_IRUSR) buf[0] += 4; 
	if (mode & S_IWUSR) buf[0] += 2; 
	if (mode & S_IXUSR) buf[0] += 1; 
	if (mode & S_IRGRP) buf[1] += 4; 
	if (mode & S_IWGRP) buf[1] += 2; 
	if (mode & S_IXGRP) buf[1] += 1; 
	if (mode & S_IROTH) buf[2] += 4; 
	if (mode & S_IWOTH) buf[2] += 2; 
	if (mode & S_IXOTH) buf[2] += 1; 
}
/*
* d_or_f
*
* Returns string "regular file" or "directory" depending on what the given file mode is.
*
* Args:
* buf: Buffer for output
* mode: A st_mode value for a file
*/
void d_or_f(char *buf, mode_t mode) {
	if(S_ISDIR(mode)) { // is this a directory?
		strcpy(buf, "directory");
	}
	else { // this is a file
		strcpy(buf, "regular file");
	}
}

/*
* tail
*
* Writes the last 10 lines of the file to stdout.
* If more than one file is given, each output is
* prefixed by a header with the file name.
* 
* Args:
* args: array of strings containing "tail" and given file names
* argcp: number of elements in args array
*/
static void cmdB_tail(char **args, int argcp) {
	if (argcp < 2) { // use case check
		fprintf(stderr, TAIL_USAGE);
    	errno = 0;
		return;
	}

	int fd;
	int line_count = 0;
	int chars_read = 0;
	off_t offset;
	char this_char;
	char *buf;

	for (int i = 1; i < argcp; i++) { // Write the last 10 lines of each given file. if more than one file, put header with file name.
		if(argcp > 2) { // print header for file when more than one file
			printf("==> %s <==\n", args[i]);
		}
		fd = open(args[i], O_RDONLY);
		if (fd == -1) { // err check
			fprintf(stderr, "\"%s\": ", args[i]);
			perror(OPEN_ERR);
			return;
		}

		offset = lseek(fd, 0, SEEK_END); // go to EOF
		if (offset == -1) { // err check
			fprintf(stderr, "\"%s\": ", args[i]);
			perror(LSEEK_ERR);
			return;
		}
		
		this_char = '\0'; // EOF cannot be read, so initialize with null terminator

		while (line_count != 10 && offset > 0) { // find which comes first, 10th line back or start of file
			if (read(fd, &this_char, 1) == -1) { // read char then move offset back -2 for backwards reading and err check
				fprintf(stderr, "\"%s\": ", args[i]);
				perror(READ_ERR);
				return;
			}
			chars_read++;
			offset = lseek(fd, -2, SEEK_CUR);
			if (offset == -1) { // err check
				fprintf(stderr, "\"%s\": ", args[i]);
				perror(LSEEK_ERR);
				return;
			}

			if (this_char == '\n') { // start of line, increment count
				line_count++;
				if (line_count == 10) { // move offset forward by 2 to point at start of 10th line from end, dec chars_read
					chars_read--;
					offset = lseek(fd, 2, SEEK_CUR);
					if (offset == -1) { // err check
						fprintf(stderr, "\"%s\": ", args[i]);
						perror(LSEEK_ERR);
						return;
					}
				}
			}
		}

		// allocate buffer based on total # of chars read, + 1 for \0
		if ((buf = malloc((chars_read) * sizeof(char))) == NULL) { // err check
			fprintf(stderr, "\"%s\": ", args[i]);
			perror(MALLOC_ERR);
			errno = 0;
			return;
		}

		if (read(fd, buf, chars_read) == -1) { // Save last 10 lines to buf
			fprintf(stderr, "\"%s\": ", args[i]);
			perror(READ_ERR);
			return;
		}

		if (write(STDOUT_FILENO, buf, chars_read) == -1) { // Write last buf to stdout
			fprintf(stderr, "\"%s\": ", args[i]);
			perror(WRITE_ERR);
			return;
		}
		printf("\n");

		// free space
		free(buf);
		if (close(fd) == -1) { // Close file and err check
			fprintf(stderr, "\"%s\": ", args[i]);
			perror(CLOSE_ERR);
			return;
		}

		line_count = 0;
		chars_read = 0;
	}
	
}

/*
* touch
*
* Sets the modification and access times of a file. Creates the file
* if it does not exist. Does not make a directory.
* 
* Args:
* args: array of strings containing "touch" and given file names
*/
static void cmdB_touch(char **args, int argcp) {
	if (argcp < 2) { // use case check
		fprintf(stderr, TOUCH_USAGE);
    	errno = 0;
		return;
	}

	int fd;

	for (int i = 1; i < argcp; i++) { // touch every file given
		fd = open(args[i], O_RDONLY | O_CREAT, 0664); // open file. Make it if it does not exist
		if (fd == -1) { // err check
			fprintf(stderr, "\"%s\": ", args[i]);
			perror(OPEN_ERR);
			return;
		}
		
		//update access and modify times
		if (utime(args[i], NULL) == -1) { // err check
			perror(TIME_ERR);
			errno = 0;
			return;
		}

		if (close(fd) == -1) { // Close file and err check
			fprintf(stderr, "\"%s\": ", args[i]);
			perror(CLOSE_ERR);
			return;
		}

		// free space
	}
}
