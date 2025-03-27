#include <CL/cl.h>
#include <stdio.h>
#include <stdlib.h>

#define CHECK_ERROR(err) if(err != CL_SUCCESS){ fprintf(stderr, "Error: %d\n", err); exit(1); }

int main(void) {
    cl_int err;
    cl_platform_id platform;
    cl_device_id device;
    cl_context context;
    cl_command_queue queue;
    cl_program program;
    cl_kernel kernel;
    cl_event event;

    // ... (initialize OpenCL: select platform, device, create context) ...

    // Create a command queue with profiling enabled
    queue = clCreateCommandQueue(context, device, CL_QUEUE_PROFILING_ENABLE, &err);
    CHECK_ERROR(err);

    // Assume program and kernel have been created and built already
    // Enqueue the kernel with an event to track profiling info
    size_t global_work_size[1] = {1024};
    err = clEnqueueNDRangeKernel(queue, kernel, 1, NULL, global_work_size, NULL, 0, NULL, &event);
    CHECK_ERROR(err);

    // Wait for kernel to finish
    clFinish(queue);

    // Query profiling information
    cl_ulong time_start, time_end;
    err = clGetEventProfilingInfo(event, CL_PROFILING_COMMAND_START, sizeof(time_start), &time_start, NULL);
    CHECK_ERROR(err);
    err = clGetEventProfilingInfo(event, CL_PROFILING_COMMAND_END, sizeof(time_end), &time_end, NULL);
    CHECK_ERROR(err);

    // Calculate the kernel execution time in nanoseconds
    cl_ulong execution_time = time_end - time_start;
    printf("Kernel execution time: %lu nanoseconds\n", execution_time);

    // Clean up
    clReleaseEvent(event);
    clReleaseCommandQueue(queue);
    clReleaseKernel(kernel);
    clReleaseProgram(program);
    clReleaseContext(context);

    return 0;
}
