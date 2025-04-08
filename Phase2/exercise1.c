// matrix_add.c
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <CL/cl.h>

#define SIZE 100
#define MAX_SOURCE_SIZE 16384

// C implementation
void add_matrix_c(float *a, float *b, float *result) {
    for(int i=0; i<SIZE*SIZE; i++) {
        result[i] = a[i] + b[i];
    }
}

// OpenCL implementation
void add_matrix_opencl(float *a, float *b, float *result) {
    cl_platform_id platform;
    cl_device_id device;
    cl_context context;
    cl_command_queue queue;
    cl_program program;
    cl_kernel kernel;
    cl_mem a_buff, b_buff, res_buff;
    cl_int err;
    cl_queue_properties properties[] = { CL_QUEUE_PROPERTIES, CL_QUEUE_PROFILING_ENABLE, 0 };

    // Get platform/device
    clGetPlatformIDs(1, &platform, NULL);
    clGetDeviceIDs(platform, CL_DEVICE_TYPE_GPU, 1, &device, NULL);

    // Create context and queue
    context = clCreateContext(NULL, 1, &device, NULL, NULL, &err);
    queue = clCreateCommandQueueWithProperties(context, device, properties, &err);

    // Create buffers
    a_buff = clCreateBuffer(context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, 
                           sizeof(float)*SIZE*SIZE, a, &err);
    b_buff = clCreateBuffer(context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, 
                           sizeof(float)*SIZE*SIZE, b, &err);
    res_buff = clCreateBuffer(context, CL_MEM_WRITE_ONLY, 
                            sizeof(float)*SIZE*SIZE, NULL, &err);

    // Create kernel
    const char *kernel_source =
    "__kernel void add_matrix(__global float *a, __global float *b, __global float *result) {"
    "    int idx = get_global_id(0);"
    "    result[idx] = a[idx] + b[idx];"
    "}";
    
    program = clCreateProgramWithSource(context, 1, &kernel_source, NULL, &err);
    clBuildProgram(program, 1, &device, NULL, NULL, NULL);
    kernel = clCreateKernel(program, "add_matrix", &err);

    // Set args and execute
    clSetKernelArg(kernel, 0, sizeof(cl_mem), &a_buff);
    clSetKernelArg(kernel, 1, sizeof(cl_mem), &b_buff);
    clSetKernelArg(kernel, 2, sizeof(cl_mem), &res_buff);
    
    size_t global_size = SIZE*SIZE;
    cl_event event;
    clEnqueueNDRangeKernel(queue, kernel, 1, NULL, &global_size, NULL, 0, NULL, &event);
    clFinish(queue);
    
    // Read results
    clEnqueueReadBuffer(queue, res_buff, CL_TRUE, 0, sizeof(float)*SIZE*SIZE, result, 0, NULL, NULL);

    // Cleanup
    clReleaseMemObject(a_buff);
    clReleaseMemObject(b_buff);
    clReleaseMemObject(res_buff);
    clReleaseKernel(kernel);
    clReleaseProgram(program);
    clReleaseCommandQueue(queue);
    clReleaseContext(context);
}

double get_time() {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (double)tv.tv_sec + (double)tv.tv_usec / 1000000.0;
}

int main() {
    float *a = (float*)malloc(SIZE*SIZE*sizeof(float));
    float *b = (float*)malloc(SIZE*SIZE*sizeof(float));
    float *result_c = (float*)malloc(SIZE*SIZE*sizeof(float));
    float *result_cl = (float*)malloc(SIZE*SIZE*sizeof(float));

    // Initialize matrices
    for(int i=0; i<SIZE*SIZE; i++) {
        a[i] = (float)i;
        b[i] = (float)i*2;
    }

    // C version
    double start = get_time();
    add_matrix_c(a, b, result_c);
    printf("C version time: %.4f ms\n", (get_time() - start)*1000);

    // OpenCL version
    start = get_time();
    add_matrix_opencl(a, b, result_cl);
    printf("OpenCL version time: %.4f ms\n", (get_time() - start)*1000);

    // Verify results
    for(int i=0; i<SIZE*SIZE; i++) {
        if(result_c[i] != result_cl[i]) {
            printf("Mismatch at %d: C=%.2f vs OpenCL=%.2f\n", i, result_c[i], result_cl[i]);
            break;
        }
    }

    free(a);
    free(b);
    free(result_c);
    free(result_cl);
    return 0;
}