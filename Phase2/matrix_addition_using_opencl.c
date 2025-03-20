#include <CL/cl.h>
#include <stdio.h>
#include <stdlib.h>

#define SIZE 100

const char *kernelSource = "__kernel void add_matrix(__global float *matrix1, __global float *matrix2, __global float *result) { \n"
                           "    int id = get_global_id(0); \n"
                           "    result[id] = matrix1[id] + matrix2[id]; \n"
                           "} \n";

int main() {
    int size = SIZE * SIZE;
    float *matrix1 = (float *)malloc(size * sizeof(float));
    float *matrix2 = (float *)malloc(size * sizeof(float));
    float *result = (float *)malloc(size * sizeof(float));

    // Initialize matrices with some values
    for (int i = 0; i < size; i++) {
        matrix1[i] = (float)i;
        matrix2[i] = (float)(i * 2);
    }

    // OpenCL setup
    cl_platform_id platform;
    cl_device_id device;
    cl_context context;
    cl_command_queue queue;
    cl_program program;
    cl_kernel kernel;
    cl_mem buffer1, buffer2, bufferResult;
    cl_int err;

    // Get platform and device
    clGetPlatformIDs(1, &platform, NULL);
    clGetDeviceIDs(platform, CL_DEVICE_TYPE_GPU, 1, &device, NULL);

    // Create context and command queue
    context = clCreateContext(NULL, 1, &device, NULL, NULL, &err);
    queue = clCreateCommandQueue(context, device, CL_QUEUE_PROFILING_ENABLE, &err);

    // Create buffers
    buffer1 = clCreateBuffer(context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, size * sizeof(float), matrix1, &err);
    buffer2 = clCreateBuffer(context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, size * sizeof(float), matrix2, &err);
    bufferResult = clCreateBuffer(context, CL_MEM_WRITE_ONLY, size * sizeof(float), NULL, &err);

    // Create program and kernel
    program = clCreateProgramWithSource(context, 1, &kernelSource, NULL, &err);
    clBuildProgram(program, 1, &device, NULL, NULL, NULL);
    kernel = clCreateKernel(program, "add_matrix", &err);

    // Set kernel arguments
    clSetKernelArg(kernel, 0, sizeof(cl_mem), &buffer1);
    clSetKernelArg(kernel, 1, sizeof(cl_mem), &buffer2);
    clSetKernelArg(kernel, 2, sizeof(cl_mem), &bufferResult);

    // Execute kernel
    size_t globalSize = size;
    cl_event event;
    clEnqueueNDRangeKernel(queue, kernel, 1, NULL, &globalSize, NULL, 0, NULL, &event);

    // Wait for kernel to finish
    clFinish(queue);

    // Profile kernel execution time
    cl_ulong start, end;
    clGetEventProfilingInfo(event, CL_PROFILING_COMMAND_START, sizeof(start), &start, NULL);
    clGetEventProfilingInfo(event, CL_PROFILING_COMMAND_END, sizeof(end), &end, NULL);
    double deviceTime = (end - start) * 1e-9; // Convert nanoseconds to seconds

    printf("Device Execution Time: %f seconds\n", deviceTime);

    // Read result back to host
    clEnqueueReadBuffer(queue, bufferResult, CL_TRUE, 0, size * sizeof(float), result, 0, NULL, NULL);

    // Cleanup
    clReleaseMemObject(buffer1);
    clReleaseMemObject(buffer2);
    clReleaseMemObject(bufferResult);
    clReleaseKernel(kernel);
    clReleaseProgram(program);
    clReleaseCommandQueue(queue);
    clReleaseContext(context);

    free(matrix1);
    free(matrix2);
    free(result);

    return 0;
}