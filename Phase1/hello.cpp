#include <stdio.h>
#include <stdlib.h>
#include <CL/cl.h>

const char* kernel_source =
"__kernel void hello(__global char *output) {"
"output[0]='h';"
"output[1]='e';"
"output[2]='l';"
"output[3]='l';"
"output[4]='o';"
"output[5]=',';"
"output[6]=' ';"
"output[7]='w';"
"output[8]='o';"
"output[9]='r';"
"output[10]='l';"
"output[11]='d';"
"output[12]='\\0';"
"}";

int main()
{
    cl_int           err;
    cl_uint          num_platforms;
    cl_platform_id* platforms;
    cl_device_id     device;
    cl_context       context;
    cl_command_queue queue;
    cl_program       program;
    cl_kernel        kernel;
    cl_mem           output;

    char result[13];

    // PLATFORM
    int num_max_platforms = 1;
    err = clGetPlatformIDs(num_max_platforms, NULL, &num_platforms);
    printf("Num platforms detected: %d\n", num_platforms);

    platforms = (cl_platform_id*)malloc(sizeof(cl_platform_id) * num_platforms);
    if (platforms == NULL) {
        printf("Failed to allocate memory for platforms.\n");
        exit(1);
    }
    err = clGetPlatformIDs(num_max_platforms, platforms, &num_platforms);

    if (num_platforms < 1)
    {
        printf("No platform detected, exit\n");
        exit(1);
    }

    // DEVICE
    err = clGetDeviceIDs(platforms[0], CL_DEVICE_TYPE_ALL, 1, &device, NULL);

    // CONTEXT
    context = clCreateContext(NULL, 1, &device, NULL, NULL, &err);

    // QUEUE (using non-deprecated function)
    queue = clCreateCommandQueueWithProperties(context, device, NULL, &err);

    // READ KERNEL AND COMPILE IT
    program = clCreateProgramWithSource(context, 1, &kernel_source, NULL, &err);
    err = clBuildProgram(program, 0, NULL, NULL, NULL, NULL);

    // CREATE KERNEL AND KERNEL PARAMETERS
    kernel = clCreateKernel(program, "hello", &err);
    output = clCreateBuffer(context, CL_MEM_WRITE_ONLY, 13 * sizeof(char), NULL, &err);
    err = clSetKernelArg(kernel, 0, sizeof(cl_mem), &output);

    // EXECUTE KERNEL (using non-deprecated function)
    size_t global_work_size = 1;
    err = clEnqueueNDRangeKernel(queue, kernel, 1, NULL, &global_work_size, NULL, 0, NULL, NULL);

    // READ KERNEL OUTPUT
    err = clEnqueueReadBuffer(queue, output, CL_TRUE, 0, 13 * sizeof(char), result, 0, NULL, NULL);
    printf("***%s***", result);

    // CLEANUP
    clReleaseMemObject(output);
    clReleaseKernel(kernel);
    clReleaseProgram(program);
    clReleaseCommandQueue(queue);
    clReleaseContext(context);
    free(platforms);

    return 0;
}


// Fist build the cpp file using ctrl + shift + B
// run the .exe file -> ./hello.exe in the terminal