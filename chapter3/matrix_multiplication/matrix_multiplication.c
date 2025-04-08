#define _CRT_SECURE_NO_WARNINGS
#define PROGRAM_FILE "blank.cl"
#define KERNEL_FUNC "blank"

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <string.h>

#ifdef MAC
#include <OpenCL/cl.h>
#else
#include <CL/cl.h>
#endif

/* Find a GPU or CPU associated with the first available platform */
cl_device_id create_device() {

   cl_platform_id platform;
   cl_device_id dev;
   int err;

   /* Identify a platform */
   err = clGetPlatformIDs(1, &platform, NULL);
   if(err < 0) {
      perror("Couldn't identify a platform");
      exit(1);
   } 

   /* Access a device */
   err = clGetDeviceIDs(platform, CL_DEVICE_TYPE_GPU, 1, &dev, NULL);
   if(err == CL_DEVICE_NOT_FOUND) {
      err = clGetDeviceIDs(platform, CL_DEVICE_TYPE_CPU, 1, &dev, NULL);
   }
   if(err < 0) {
      perror("Couldn't access any devices");
      exit(1);   
   }

   return dev;
}

/* Create program from a file and compile it */
cl_program build_program(cl_context ctx, cl_device_id dev, const char* filename) {

   cl_program program;
   FILE *program_handle;
   char *program_buffer, *program_log;
   size_t program_size, log_size;
   int err;

   /* Read program file and place content into buffer */
   program_handle = fopen(filename, "r");
   if(program_handle == NULL) {
      perror("Couldn't find the program file");
      exit(1);
   }
   fseek(program_handle, 0, SEEK_END);
   program_size = ftell(program_handle);
   rewind(program_handle);
   program_buffer = (char*)malloc(program_size + 1);
   program_buffer[program_size] = '\0';
   fread(program_buffer, sizeof(char), program_size, program_handle);
   fclose(program_handle);

   /* Create program from file */
   program = clCreateProgramWithSource(ctx, 1, 
      (const char**)&program_buffer, &program_size, &err);
   if(err < 0) {
      perror("Couldn't create the program");
      exit(1);
   }
   free(program_buffer);

   /* Build program */
   err = clBuildProgram(program, 0, NULL, NULL, NULL, NULL);
   if(err < 0) {

      /* Find size of log and print to std output */
      clGetProgramBuildInfo(program, dev, CL_PROGRAM_BUILD_LOG, 
            0, NULL, &log_size);
      program_log = (char*) malloc(log_size + 1);
      program_log[log_size] = '\0';
      clGetProgramBuildInfo(program, dev, CL_PROGRAM_BUILD_LOG, 
            log_size + 1, program_log, NULL);
      printf("%s\n", program_log);
      free(program_log);
      exit(1);
   }

   return program;
}

int main() {

   /* OpenCL data structures */
   cl_device_id device;
   cl_context context;
   cl_command_queue queue;
   cl_program program;
   cl_kernel kernel;
   cl_int i, j, err;

   /* Data and buffers */
   float data_one[100], data_two[100], result_array[100];
   cl_mem buffer_one, buffer_two;
   void* mapped_memory;

   /* Initialize arrays */
   for(i=0; i<100; i++) {
      data_one[i] = 1.0f*i;
      data_two[i] = -1.0f*i;
      result_array[i] = 0.0f;
   }

   /* Display Data one */
   printf("Display data one:\n");
   for(i=0; i<10; i++) {
    for(j=0; j<10; j++) {
       printf("%6.1f", data_one[j+i*10]);
    }
    printf("\n");
 }
 
 /* Display Data two */
 printf("Display data two:\n");
 for(i=0; i<10; i++) {
    for(j=0; j<10; j++) {
       printf("%6.1f", data_two[j+i*10]);
    }
    printf("\n");
 }

 /* Display Result */
 printf("Display Result:\n");
 for(i=0; i<10; i++) {
    for(j=0; j<10; j++) {
       printf("%6.1f", result_array[j+i*10]);
    }
    printf("\n");
 }

   /* Create a device and context */
   device = create_device();
   context = clCreateContext(NULL, 1, &device, NULL, NULL, &err);
   if(err < 0) {
      perror("Couldn't create a context");
      exit(1);   
   }


   /* Create buffers */
   buffer_one = clCreateBuffer(context, CL_MEM_READ_WRITE | 
         CL_MEM_COPY_HOST_PTR, sizeof(data_one), data_one, &err);
   if(err < 0) {
      perror("Couldn't create a buffer object");
      exit(1);   
   }
   buffer_two = clCreateBuffer(context, CL_MEM_READ_WRITE | 
         CL_MEM_COPY_HOST_PTR, sizeof(data_two), data_two, NULL);

   /*Display the buffer content*/

   /* Create a command queue */
   queue = clCreateCommandQueue(context, device, 0, &err);
   if(err < 0) {
      perror("Couldn't create a command queue");
      exit(1);   
   };


   /* Enqueue command to copy buffer one to buffer two */
   err = clEnqueueCopyBuffer(queue, buffer_one, buffer_two, 0, 0,
         sizeof(data_one), 0, NULL, NULL); 
   if(err < 0) {
      perror("Couldn't perform the buffer copy");
      exit(1);   
   }

   /* Enqueue command to map buffer two to host memory */
   mapped_memory = clEnqueueMapBuffer(queue, buffer_two, CL_TRUE,
         CL_MAP_READ, 0, sizeof(data_two), 0, NULL, NULL, &err);
   if(err < 0) {
      perror("Couldn't map the buffer to host memory");
      exit(1);   
   }

   /* Transfer memory and unmap the buffer */
   memcpy(result_array, mapped_memory, sizeof(data_two));
   err = clEnqueueUnmapMemObject(queue, buffer_two, mapped_memory,
         0, NULL, NULL);
   if(err < 0) {
      perror("Couldn't unmap the buffer");
      exit(1);   
   }

   /* Display updated buffer */
   printf("Display the copied buffer in result array:\n");
   for(i=0; i<10; i++) {
      for(j=0; j<10; j++) {
         printf("%6.1f", result_array[j+i*10]);
      }
      printf("\n");
   }

   /* Deallocate resources */
   clReleaseMemObject(buffer_one);
   clReleaseMemObject(buffer_two);
   clReleaseKernel(kernel);
   clReleaseCommandQueue(queue);
   clReleaseProgram(program);
   clReleaseContext(context);

   return 0;
}
