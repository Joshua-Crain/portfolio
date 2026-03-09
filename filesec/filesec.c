/* Name: Joshua Crain
*  Class: CSCI 347
*  Date: 10/6/2025
*  Description: Program that takes a -e or -d flag for encryption or
*  decryption (+ or - 100 to ASCII of chars respectively) for a given
*  file. Creates a file with _enc or _dec after the name with the changes.
*/
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/time.h>
#include <stdlib.h>

#define BUFSIZE 5 // buffer size for read/write
#define OPENERR_MSG "Error opening/creating file\n"
#define READERR_MSG "Error reading file\n"
#define WRITEERR_MSG "Error writing to file\n"
#define CLOSEERR_MSG "Error closing files\n"
#define USAGEERR_MSG "Usage: \n ./filesec -e|-d [filename.txt]\n"

// num_to_str: Converts a given number into a string,
// used to count read & write call pairs.
// Takes in number to convert and a buffer.
// Returns number of digits
int num_to_str(long int number, char* buffer) {
    int i = 0, j = 0, digits;
    char temp;
    while(number > 0) { // parse number, in reverse order
        buffer[i++] = number % 10 + '0'; // get ASCII for digit
        number /= 10; 
    }

    digits = i;
    i--;
    while(i > j) { // un-reverse order
        temp = buffer[i];
        buffer[i] = buffer[j];
        buffer[j] = temp;
        i--;
        j++;
    }

    return digits;
};

// print_start_info: Write start time to STDOUT
// takes a struct timeval, required for gettimeofday()
// writes the start time in seconds and end time in
// microseconds to STDOUT.
void print_start_info(struct timeval start_time) {
    char *num_buf;
    int bytes;
    gettimeofday(&start_time, NULL);
    num_buf = malloc(sizeof(char) * 20); // long int is < 20 digits max
    bytes = num_to_str((long int)start_time.tv_sec, num_buf);
    write(STDOUT_FILENO, num_buf, bytes);
    write(STDOUT_FILENO, " ", 1);
    bytes = num_to_str((long int)start_time.tv_usec, num_buf);
    write(STDOUT_FILENO, num_buf, bytes);
    write(STDOUT_FILENO, "\n", 1);
    free(num_buf);
}

// print_end_info: Write end time to STDOUT and read & write pair count
// takes a struct timeval, required for gettimeofday()
// and the read & write counter
void print_end_info(struct timeval end_time, int count_read_write) {
    char *num_buf;
    int bytes;
    gettimeofday(&end_time, NULL);
    num_buf = malloc(sizeof(char) * 20); // long int is < 20 digits max
    bytes = num_to_str((long int)end_time.tv_sec, num_buf);
    write(STDOUT_FILENO, num_buf, bytes);
    write(STDOUT_FILENO, " ", 1);
    bytes = num_to_str((long int)end_time.tv_usec, num_buf);
    write(STDOUT_FILENO, num_buf, bytes);
    write(STDOUT_FILENO, " ", 1);
    bytes = num_to_str(count_read_write, num_buf);
    write(STDOUT_FILENO, num_buf, bytes);
    write(STDOUT_FILENO, "\n", 1);
    free(num_buf);
};


int main(int argc, char** argv)
{
    struct timeval start_time, end_time;
    char output_file_name[128];    //You may assume that the length of the output file name will not exceed 128 characters.
    char *buffer;
    int out_fd, in_fd, read_count;
    int count_read_write = 0;

    if (argc != 3) { // improper use case
        write(STDERR_FILENO, USAGEERR_MSG, sizeof(USAGEERR_MSG));
        return -1;
    } 

    print_start_info(start_time);

    buffer = malloc(sizeof(char) * BUFSIZE);
    in_fd = open(argv[2], O_RDONLY);

    if (in_fd < 0) { // err check
        write(STDERR_FILENO, READERR_MSG, sizeof(READERR_MSG));
        free(buffer);
        return -1;
    }
    strcpy(output_file_name, strtok(argv[2], "."));

    if (strcmp(argv[1], "-e") == 0) { // encrypt
        strcat(output_file_name, "_enc.txt");
        out_fd = open(output_file_name, O_WRONLY | O_CREAT | O_TRUNC, 0644);
        if (out_fd < 0) { // err check
        write(STDERR_FILENO, OPENERR_MSG, sizeof(OPENERR_MSG));
            free(buffer);
            return -1;
        }

        while ((read_count = read(in_fd, buffer, BUFSIZE))) { // read, encrypt, write
            for(int i = 0; i < read_count; i++) {
                buffer[i] = buffer[i] + 100; // encryption
            }
            count_read_write++;
            if(write(out_fd, buffer, read_count) == -1) {
                write(STDERR_FILENO, WRITEERR_MSG, sizeof(WRITEERR_MSG));
                free(buffer);
                return -1;
            }
        }
        if(close(in_fd) == 0 && close(out_fd) == 0) { // done if closed!
            print_end_info(end_time, count_read_write);
            free(buffer);
            return 1;
        }
        else {
            write(STDERR_FILENO, CLOSEERR_MSG, sizeof(CLOSEERR_MSG));
            free(buffer);
            return -1;
        }
    }
    else if (strcmp(argv[1], "-d") == 0) { // decrypt
        strcat(output_file_name, "_dec.txt");
        out_fd = open(output_file_name, O_WRONLY | O_CREAT | O_TRUNC, 0644);
        if (out_fd < 0) { // err check
            write(STDERR_FILENO, OPENERR_MSG, sizeof(OPENERR_MSG));
            free(buffer);
            return -1;
        }

        while ((read_count = read(in_fd, buffer, BUFSIZE))) { // read, decrypt, write
            for(int i = 0; i < read_count; i++) {
                buffer[i] = buffer[i] - 100; // decryption
            }
            count_read_write++;
            if(write(out_fd, buffer, read_count) == -1) {
                write(STDERR_FILENO, WRITEERR_MSG, sizeof(WRITEERR_MSG));
                free(buffer);
                return -1;
            }
        }
        if(close(in_fd) == 0 && close(out_fd) == 0) { // done if closed!
            print_end_info(end_time, count_read_write);
            free(buffer);
            return 1;
        }
        else {
            write(STDERR_FILENO, CLOSEERR_MSG, sizeof(CLOSEERR_MSG));
            free(buffer);
            return -1;
        }
    }
    else { // improper flag
        write(STDERR_FILENO, USAGEERR_MSG, sizeof(USAGEERR_MSG));
        free(buffer);
        return -1;
    }
}
