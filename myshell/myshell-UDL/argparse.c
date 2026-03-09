/* Name: Joshua Crain
 * Date: 11/1/2025
 */
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include "argparse.h"
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/wait.h>
#include <errno.h>

#define FALSE (0)
#define TRUE (1)

#define MALLOC_ERR "Error allocating space for argument\n"

/*
 * argCount
 *
 * Counts the number of arguments in a given input line.
 * You may assume only whitespace is used to separate arguments.
 * argCount should be able to handle multiple whitespaces between arguments.
 *
 * Args:
 *   line: intput string containing command and arguments separated by whitespaces
 *
 * Returns:
 *   The number of arguments in line.
 *   (The command itself counts as the first argument)
 *
 * Example:
 *   argCount("ls -l /home") returns 3
 *   argCount("   ls    -l   /home  ") returns 3
 */
static int argCount(char *line) {
  int count = 0;
  int arg_flag = 1; // flag: 1 = last read white space, 0 = currently on an argument

  while (*line != '\0') { // iterate through whole command line argument
    if (isspace(*line)) { // skip white space, marking that a white space was last read
      arg_flag = 1;
      line++;
    } 
    else { // non-white space, this is an argument
      if (arg_flag == 1) { // this is a new argument
        count++;
      }
      arg_flag = 0;
      line++;
    }
  }
  return count;
}

/*
 * argparse
 *
 * Parses an input line into an array of strings.
 *
 *
 * You may assume only whitespace is used to separate strings.
 * argparse should be able to handle multiple whitespaces between strings.
 * The function should dynamically allocate space for the array of strings,
 * following the project requirements.
 *
 * Args:
 *   line: input string containing words separated by whitespace
 *   argcp: stores the number of strings in the line
 *
 * Returns:
 *   An array of strings.
 *
 * Example:
 *   argparse("ls -l /home", &argc) --> returns ["ls", "-l", "/home", NULL] and set argc to 3
 *   argparse("   ls    -l   /home  ", &argc) --> returns ["ls", "-l", "/home", NULL] and set argc to 3
 */
char **argparse(char *line, int *argcp) {
  char **args, **args_ptr;
  char *line_start, *arg_start; // references to track location in line
  int arg_flag = 1; // flag: 1 = last read white space, 0 = currently on an argument
  
  int count = argCount(line);
  *argcp = count;
  args = malloc((count + 1) * sizeof(char *));
  if (args == NULL) { // err check
		perror(MALLOC_ERR);
    errno = 0;
		return NULL;
	}
  
  args_ptr = args;

  line_start = line;
  arg_start = line;
  while (*line != '\0') { // iterate through whole command line argument
    if (isspace(*line)) { // skip white space, marking that a white space was last read
      if (arg_flag == 0) { // end of previous argument? Save it & reuse same memory from line
        *line = '\0';
        *args_ptr = arg_start;
        args_ptr++;
      }
      
      arg_flag = 1;
      line++;
      arg_start = line;
    } 
    else { // non-white space, move line forward
      arg_flag = 0;
      line++;
    }
  }
  if (arg_flag == 1) { // trailing white space? No final argument to save
    *args_ptr = NULL; // final NULL to end array of char*
  }
  else {// save final argument, then return array of strings
    *line = '\0';
    *args_ptr = arg_start;
    args_ptr++;
    *args_ptr = NULL; // final NULL to end array of char*
  }
  return args;
}
