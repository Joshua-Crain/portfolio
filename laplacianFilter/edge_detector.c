/* Name: Joshua Crain
*  Date: 11/24/2025
*  Project 3 - Edge Detector
*/
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <sys/time.h>
#include <pthread.h>
#include <string.h>

#define LAPLACIAN_THREADS 4     //change the number of threads as you run your concurrency experiment

/* Laplacian filter is 3 by 3 */
#define FILTER_WIDTH 3       
#define FILTER_HEIGHT 3      

#define RGB_COMPONENT_COLOR 255

typedef struct {
      unsigned char r, g, b;
} PPMPixel;

struct parameter {
    PPMPixel *image;         //original image pixel data
    PPMPixel *result;        //filtered image pixel data
    unsigned long int w;     //width of image
    unsigned long int h;     //height of image
    unsigned long int start; //starting point of work
    unsigned long int size;  //equal share of work (almost equal if odd)
};


struct file_name_args {
    char *input_file_name;      //e.g., file1.ppm
    char output_file_name[20];  //will take the form laplaciani.ppm, e.g., laplacian1.ppm
};


/*The total_elapsed_time is the total time taken by all threads 
to compute the edge detection of all input images .
*/
double total_elapsed_time = 0; 

// mutex lock
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

/*This is the thread function. It will compute the new values for the region of image specified in params (start to start+size) using convolution.
    For each pixel in the input image, the filter is conceptually placed on top of the image with its origin lying on that pixel.
    The  values  of  each  input  image  pixel  under  the  mask  are  multiplied  by the corresponding filter values.
    Truncate values smaller than zero to zero and larger than 255 to 255.
    The results are summed together to yield a single output value that is placed in the output image at the location of the pixel being processed on the input.
 
 */
void *compute_laplacian_threadfn(void *params) {
	int laplacian[FILTER_WIDTH][FILTER_HEIGHT] =
	{
		{-1, -1, -1},
		{-1,  8, -1},
		{-1, -1, -1}
	};

	int red, green, blue;
    struct parameter *param_struct = params;
    unsigned long int end = param_struct->start + param_struct->size; // ending row for iteration
    unsigned long int x_coordinate, y_coordinate;

    // update [start, size) pixels in image, and save filtered pixel data to result in same index
    for(unsigned long int y = param_struct->start; y < end; y++) { // row
        for(unsigned long int x = 0; x < param_struct->w; x++) { // column
            red = 0;
            green = 0;
            blue = 0;

            // filter iteration for convolution
            for(int filter_y = 0; filter_y < FILTER_HEIGHT; filter_y++) { // filter row
                for(int filter_x = 0; filter_x < FILTER_WIDTH; filter_x++) { // filter column
                    x_coordinate = (x - FILTER_WIDTH / 2 + filter_x + param_struct->w) % param_struct->w;
                    y_coordinate = (y - FILTER_HEIGHT / 2 + filter_y + param_struct->h) % param_struct->h;

                    red += param_struct->image[y_coordinate * param_struct->w + x_coordinate].r * laplacian[filter_y][filter_x];
                    green += param_struct->image[y_coordinate * param_struct->w + x_coordinate].g * laplacian[filter_y][filter_x];
                    blue += param_struct->image[y_coordinate * param_struct->w + x_coordinate].b * laplacian[filter_y][filter_x];
                }
            }

            // create result pixel data
            if(red < 0) {
                red = 0;
            }
            else if(red > 255) { 
                red = 255;
            }
            if(green < 0) {
                green = 0;
            }
            else if(green > 255) {
                green = 255;
            }
            if(blue < 0) {
                blue = 0;
            }
            else if(blue > 255) {
                blue = 255;
            }
            param_struct->result[y * param_struct->w + x].r = red;
            param_struct->result[y * param_struct->w + x].g = green;
            param_struct->result[y * param_struct->w + x].b = blue;
        }
    }
    
	return NULL;
}

/* Apply the Laplacian filter to an image using threads.
 Each thread shall do an equal share of the work, i.e. work=height/number of threads. If the size is not even, the last thread shall take the rest of the work.
 Compute the elapsed time and store it in *elapsedTime (Read about gettimeofday).
 Return: result (filtered image)
 */
PPMPixel *apply_filters(PPMPixel *image, unsigned long w, unsigned long h, double *elapsedTime) {
	int status = 0;
    pthread_t *tid_arr = calloc(LAPLACIAN_THREADS, sizeof(pthread_t));
    PPMPixel *result = calloc((w * h), sizeof(PPMPixel));
    unsigned long int work = h / LAPLACIAN_THREADS; // # of rows to work on
	unsigned long int remaining_work = h;

    // get current time
    struct timeval start_time;
    struct timeval end_time;
    double microseconds;
    gettimeofday(&start_time, NULL);

    // create parameter structs for storing calculated data
    struct parameter params[LAPLACIAN_THREADS];
	int temp = 0; // used for calculating next thread's starting point

	for(int i = 0; i < LAPLACIAN_THREADS; i++) {
		params[i].image = image;
		params[i].result = result;
		params[i].w = w;
		params[i].h = h;
		params[i].start = temp; // row to start on 

		if(remaining_work < work) { // account for uneven amounts of remaining work
			params[i].size = remaining_work;
		}
		else {
			params[i].size = work;
		}
        
		remaining_work -= work;
		temp += work;
	}

    // create threads to divide up work based on height/number of threads
    for(int i = 0; i < LAPLACIAN_THREADS; i++) {
        pthread_create(&tid_arr[i], NULL, compute_laplacian_threadfn, &params[i]);
    }

	// wait for all threads to return
	for(int i = 0; i < LAPLACIAN_THREADS; i++) {
		status = pthread_join(tid_arr[i], NULL);
		if (status != 0) { // error check
			fprintf(stderr, "thread join error for thread %d: %s\n", i+1, strerror(status));
		}
	}

    gettimeofday(&end_time, NULL);
    // get elapsed time in microseconds
    microseconds = (double)end_time.tv_usec / 1000000 + (double)end_time.tv_sec;
    microseconds -= (double)start_time.tv_usec / 1000000 + (double)start_time.tv_sec;
    
    *elapsedTime = microseconds;

    free(tid_arr);

    return result;
}

/*Create a new P6 file to save the filtered image in. Write the header block
 e.g. P6
      Width Height
      Max color value
 then write the image data.
 The name of the new file shall be "filename" (the second argument).
 */
void write_image(PPMPixel *image, char *filename, unsigned long int width, unsigned long int height)
{
    FILE *file;
    int total_pixels = width * height;
    file = fopen(filename, "w");
    if(file == NULL) { // error check
        perror("error creating/opening file");
        return;
    }
    
    // assemble file header block
    fputs("P6\n", file);
    fprintf(file, "%lu %lu\n", width, height);
    fputs("255\n", file);

    // write pixel data
    for(int i = 0; i < total_pixels; i++) {
        fputc(image[i].r, file);
        fputc(image[i].g, file);
        fputc(image[i].b, file);
    }

    fclose(file);
}



/* Open the filename image for reading, and parse it.
    Example of a ppm header:    //http://netpbm.sourceforge.net/doc/ppm.html
    P6                  -- image format
    # comment           -- comment lines begin with
    ## another comment  -- any number of comment lines
    200 300             -- image width & height 
    255                 -- max color value
 
 Check if the image format is P6. If not, print invalid format error message.
 If there are comments in the file, skip them. You may assume that comments exist only in the header block.
 Read the image size information and store them in width and height.
 Check the rgb component, if not 255, display error message.
 Return: pointer to PPMPixel that has the pixel data of the input image (filename).The pixel data is stored in scanline order from left to right (up to bottom) in 3-byte chunks (r g b values for each pixel) encoded as binary numbers.
 */
PPMPixel *read_image(const char *filename, unsigned long int *width, unsigned long int *height) {
    PPMPixel *img;
    FILE *file;
	char *buffer = calloc(20, sizeof(char));
    char char_buffer;
	char *token;
    const char* delimiter = " \n";
    int total_pixels = 0;
    
    file = fopen(filename, "r");
	if(file == NULL){ // error check
		perror("error opening image file");
        free(buffer);
		return NULL;
	}
	
	// read image format
	fgets(buffer, 4, file);
	if(strcmp(buffer, "P6\n") != 0) { // improper format check
		fprintf(stderr, "error: improper file format\n");
        free(buffer);
		return NULL;
	}

	// skip comment lines
	char_buffer = fgetc(file);
    while(char_buffer == '#') {
        do {
            char_buffer = fgetc(file);
        } while (char_buffer != '\n');
        char_buffer = fgetc(file); // check starting char at next line for a comment
    }

    // once done, move file position indicator back by one
    ungetc(char_buffer, file);

	// parse image width and height
    fgets(buffer, 20, file);
	token = strtok(buffer, delimiter);
	*width = atoi(token);
	token = strtok(NULL, delimiter);
	*height = atoi(token);

	// parse max color value
	fgets(buffer, 20, file);
	if(strcmp(buffer, "255\n") != 0) { // improper color value check
		fprintf(stderr, "error: improper color value\n");
        free(buffer);
		return NULL;
	}

    // create PPMPixel array of width * height size and get pixel data (read chunks of 3 for pixel data)
    total_pixels = *width * *height;
    img = calloc((total_pixels), sizeof(PPMPixel));
    for (int i = 0; i < total_pixels; i++){ 
        img[i].r = fgetc(file);
        img[i].g = fgetc(file);
        img[i].b = fgetc(file);
    }

    fclose(file);

    free(buffer);

    return img;
}

/* The thread function that manages an image file. 
 Read an image file that is passed as an argument at runtime. 
 Apply the Laplacian filter. 
 Update the value of total_elapsed_time.
 Save the result image in a file called laplaciani.ppm, where i is the image file order in the passed arguments.
 Example: the result image of the file passed third during the input shall be called "laplacian3.ppm".

*/
void *manage_image_file(void *args) {
    unsigned long int width = 0, height = 0;
    struct file_name_args *file_names = args;
    double elapsed_time = 0;

    PPMPixel *img = read_image(file_names->input_file_name, &width, &height);
    if(img == NULL) { // failed image read check
        pthread_exit(NULL);
    }

    // TODO: filter appliction and time calculation
    PPMPixel *new_img = apply_filters(img, width, height, &elapsed_time);

    write_image(new_img, file_names->output_file_name, width, height);

    // update time counter
    pthread_mutex_lock(&mutex);
    total_elapsed_time += elapsed_time;
    pthread_mutex_unlock(&mutex);

    free(img);
    free(new_img);

    pthread_exit(0);
}

/*The driver of the program. Check for the correct number of arguments. If wrong print the message: "Usage ./a.out filename[s]"
  It shall accept n filenames as arguments, separated by whitespace, e.g., ./a.out file1.ppm file2.ppm    file3.ppm
  It will create a thread for each input file to manage.  
  It will print the total elapsed time in .4 precision seconds(e.g., 0.1234 s). 
 */
int main(int argc, char *argv[]) {
	int status = 0;
 	pthread_t *tid_arr;
	
	if(argc == 1) { // usage check
		printf("Usage: ./edge_detector filename[s]\n");
        return -1;
	}
	
    // create file_name_args structs for each file
    struct file_name_args file_names[argc - 1];
    for(int i = 0; i < argc - 1; i++) {
        file_names[i].input_file_name = argv[i+1];
        sprintf(file_names[i].output_file_name, "laplacian%d", i+1);
        strcat(file_names[i].output_file_name, ".ppm");
    }

	// create threads for each input file calling manage_image_file
	tid_arr = calloc(argc - 1, sizeof(pthread_t));
	for(int i = 0; i < argc - 1; i++) {
		status = pthread_create(&tid_arr[i], NULL, manage_image_file, &file_names[i]);
		if (status != 0) { // error check
			fprintf(stderr, "error creating thread for arg %d: %s\n", i+1, strerror(status));
		}
	}
	
	// wait for threads to return
	for(int i = 0; i < argc - 1; i++) {
		status = pthread_join(tid_arr[i], NULL);
		if (status != 0) { // error check
			fprintf(stderr, "threadjoin error for thread %d: %s\n", i+1, strerror(status));
		}
	}    

    printf("total elapsed time: %.4lf\n", total_elapsed_time);
    
    pthread_mutex_destroy(&mutex);

    free(tid_arr);
    return 0;
}

