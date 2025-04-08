#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <CL/cl.h>
#include "lodepng.h"

#define MAX_DISP 65
#define WINDOW_SIZE 4
#define THRESHOLD 8
#define PLATFORM_ID 0
#define DEVICE_ID 0

// Image dimensions
const unsigned ORIG_WIDTH = 2940;
const unsigned ORIG_HEIGHT = 2016;
const unsigned WIDTH = 735;
const unsigned HEIGHT = 504;

// OpenCL structures
cl_context context;
cl_command_queue queue;
cl_program program;
cl_kernel resize_kernel, zncc_kernel, crosscheck_kernel;

const char* kernel_source = 
"__kernel void resize_grayscale(__global const uchar4 *input, __global uchar *output, "
"                              const uint orig_width, const uint scale) {"
"    const uint x = get_global_id(0);"
"    const uint y = get_global_id(1);"
"    const uint orig_x = x * scale;"
"    const uint orig_y = y * scale;"
"    uchar4 pixel = input[orig_y * orig_width + orig_x];"
"    output[y * get_global_size(0) + x] = convert_uchar_sat(0.2126f * pixel.x + "
"                                                         0.7152f * pixel.y + "
"                                                         0.0722f * pixel.z);"
"}"
""
"__kernel void compute_zncc(__global const uchar *left, __global const uchar *right, "
"                          __global float *disp_map, const int width, const int height, "
"                          const int max_disp, const int window_size) {"
"    const int x = get_global_id(0);"
"    const int y = get_global_id(1);"
"    float max_zncc = -INFINITY;"
"    int best_d = 0;"
"    "
"    for(int d = 0; d < max_disp; d++) {"
"        if(x - d < 0) continue;"
"        "
"        float mean_l = 0.0f, mean_r = 0.0f;"
"        int count = 0;"
"        for(int dy = -window_size; dy <= window_size; dy++) {"
"            for(int dx = -window_size; dx <= window_size; dx++) {"
"                int nx = x + dx;"
"                int ny = y + dy;"
"                if(nx >= 0 && nx < width && ny >= 0 && ny < height) {"
"                    mean_l += left[ny * width + nx];"
"                    mean_r += right[ny * width + (nx - d)];"
"                    count++;"
"                }"
"            }"
"        }"
"        mean_l /= count;"
"        mean_r /= count;"
"        "
"        float numerator = 0.0f, denom_l = 0.0f, denom_r = 0.0f;"
"        for(int dy = -window_size; dy <= window_size; dy++) {"
"            for(int dx = -window_size; dx <= window_size; dx++) {"
"                int nx = x + dx;"
"                int ny = y + dy;"
"                int nx_d = nx - d;"
"                if(nx >= 0 && nx < width && ny >= 0 && ny < height &&"
"                   nx_d >= 0 && nx_d < width) {"
"                    float l = left[ny * width + nx] - mean_l;"
"                    float r = right[ny * width + nx_d] - mean_r;"
"                    numerator += l * r;"
"                    denom_l += l * l;"
"                    denom_r += r * r;"
"                }"
"            }"
"        }"
"        "
"        float zncc = numerator / sqrt(denom_l * denom_r + 1e-6f);"
"        if(zncc > max_zncc) {"
"            max_zncc = zncc;"
"            best_d = d;"
"        }"
"    }"
"    disp_map[y * width + x] = best_d;"
"}"
""
"__kernel void cross_check(__global const float *disp_left, __global const float *disp_right,"
"                         __global float *disp_final, const int width, const int threshold) {"
"    const int x = get_global_id(0);"
"    const int y = get_global_id(1);"
"    int d = disp_left[y * width + x];"
"    if(x - d >= 0) {"
"        if(fabs(disp_left[y * width + x] - disp_right[y * width + (x - d)]) > threshold)"
"            disp_final[y * width + x] = 0;"
"        else"
"            disp_final[y * width + x] = d;"
"    } else {"
"        disp_final[y * width + x] = 0;"
"    }"
"}";

void cleanup() {
    clReleaseKernel(resize_kernel);
    clReleaseKernel(zncc_kernel);
    clReleaseKernel(crosscheck_kernel);
    clReleaseProgram(program);
    clReleaseCommandQueue(queue);
    clReleaseContext(context);
}

void check_error(cl_int err, const char *msg) {
    if(err != CL_SUCCESS) {
        fprintf(stderr, "Error %d: %s\n", err, msg);
        cleanup();
        exit(1);
    }
}

void setup_opencl() {
    cl_int err;
    cl_platform_id platform;
    cl_device_id device;
    cl_context_properties props[3] = {
        CL_CONTEXT_PLATFORM, 
        0, 
        0
    };

    // Get platform and device
    clGetPlatformIDs(1, &platform, NULL);
    clGetDeviceIDs(platform, CL_DEVICE_TYPE_GPU, 1, &device, NULL);
    props[1] = (cl_context_properties)platform;

    // Create context
    context = clCreateContext(props, 1, &device, NULL, NULL, &err);
    check_error(err, "clCreateContext");
    
    // Create command queue with properties
    cl_queue_properties q_props[] = {CL_QUEUE_PROPERTIES, CL_QUEUE_PROFILING_ENABLE, 0};
    queue = clCreateCommandQueueWithProperties(context, device, q_props, &err);
    check_error(err, "clCreateCommandQueue");

    // Create program
    program = clCreateProgramWithSource(context, 1, &kernel_source, NULL, &err);
    check_error(err, "clCreateProgramWithSource");

    // Build program
    err = clBuildProgram(program, 1, &device, NULL, NULL, NULL);
    if(err != CL_SUCCESS) {
        char log[4096];
        clGetProgramBuildInfo(program, device, CL_PROGRAM_BUILD_LOG, sizeof(log), log, NULL);
        fprintf(stderr, "Build error:\n%s\n", log);
        exit(1);
    }

    // Create kernels
    resize_kernel = clCreateKernel(program, "resize_grayscale", &err);
    check_error(err, "clCreateKernel resize");
    zncc_kernel = clCreateKernel(program, "compute_zncc", &err);
    check_error(err, "clCreateKernel zncc");
    crosscheck_kernel = clCreateKernel(program, "cross_check", &err);
    check_error(err, "clCreateKernel crosscheck");
}

void run_gpu_pipeline() {
    cl_int err;
    unsigned char *left_png = NULL, *right_png = NULL;
    float *disp_final = NULL;
    unsigned error;
    unsigned width, height;

    // Load original images
    error = lodepng_decode32_file(&left_png, &width, &height, "im0.png");
    if(error) printf("Error %u: %s\n", error, lodepng_error_text(error));
    error = lodepng_decode32_file(&right_png, &width, &height, "im1.png");
    if(error) printf("Error %u: %s\n", error, lodepng_error_text(error));

    // Create OpenCL buffers
    cl_mem d_left = clCreateBuffer(context, CL_MEM_READ_ONLY, ORIG_WIDTH*ORIG_HEIGHT*4, NULL, &err);
    check_error(err, "clCreateBuffer d_left");
    cl_mem d_right = clCreateBuffer(context, CL_MEM_READ_ONLY, ORIG_WIDTH*ORIG_HEIGHT*4, NULL, &err);
    check_error(err, "clCreateBuffer d_right");
    cl_mem d_left_gray = clCreateBuffer(context, CL_MEM_READ_WRITE, WIDTH*HEIGHT, NULL, &err);
    check_error(err, "clCreateBuffer d_left_gray");
    cl_mem d_right_gray = clCreateBuffer(context, CL_MEM_READ_WRITE, WIDTH*HEIGHT, NULL, &err);
    check_error(err, "clCreateBuffer d_right_gray");
    cl_mem d_disp_left = clCreateBuffer(context, CL_MEM_WRITE_ONLY, WIDTH*HEIGHT*sizeof(float), NULL, &err);
    check_error(err, "clCreateBuffer d_disp_left");
    cl_mem d_disp_right = clCreateBuffer(context, CL_MEM_WRITE_ONLY, WIDTH*HEIGHT*sizeof(float), NULL, &err);
    check_error(err, "clCreateBuffer d_disp_right");
    cl_mem d_disp_final = clCreateBuffer(context, CL_MEM_WRITE_ONLY, WIDTH*HEIGHT*sizeof(float), NULL, &err);
    check_error(err, "clCreateBuffer d_disp_final");

    // Upload images to GPU
    clEnqueueWriteBuffer(queue, d_left, CL_TRUE, 0, ORIG_WIDTH*ORIG_HEIGHT*4, left_png, 0, NULL, NULL);
    clEnqueueWriteBuffer(queue, d_right, CL_TRUE, 0, ORIG_WIDTH*ORIG_HEIGHT*4, right_png, 0, NULL, NULL);

    // Create temporary variables for kernel arguments
    unsigned int orig_width = ORIG_WIDTH;
    unsigned int scale = 4;
    int max_disp = MAX_DISP;
    int window_size = WINDOW_SIZE;
    int threshold = THRESHOLD;

    // Set kernel arguments for resize
    size_t global[2] = {WIDTH, HEIGHT};
    clSetKernelArg(resize_kernel, 0, sizeof(cl_mem), &d_left);
    clSetKernelArg(resize_kernel, 1, sizeof(cl_mem), &d_left_gray);
    clSetKernelArg(resize_kernel, 2, sizeof(unsigned int), &orig_width);
    clSetKernelArg(resize_kernel, 3, sizeof(unsigned int), &scale);
    
    // Process left image
    clEnqueueNDRangeKernel(queue, resize_kernel, 2, NULL, global, NULL, 0, NULL, NULL);
    
    // Process right image
    clSetKernelArg(resize_kernel, 0, sizeof(cl_mem), &d_right);
    clSetKernelArg(resize_kernel, 1, sizeof(cl_mem), &d_right_gray);
    clEnqueueNDRangeKernel(queue, resize_kernel, 2, NULL, global, NULL, 0, NULL, NULL);

    // Set up ZNCC kernel arguments
    clSetKernelArg(zncc_kernel, 0, sizeof(cl_mem), &d_left_gray);
    clSetKernelArg(zncc_kernel, 1, sizeof(cl_mem), &d_right_gray);
    clSetKernelArg(zncc_kernel, 2, sizeof(cl_mem), &d_disp_left);
    clSetKernelArg(zncc_kernel, 3, sizeof(int), &WIDTH);
    clSetKernelArg(zncc_kernel, 4, sizeof(int), &HEIGHT);
    clSetKernelArg(zncc_kernel, 5, sizeof(int), &max_disp);
    clSetKernelArg(zncc_kernel, 6, sizeof(int), &window_size);

    // Compute left disparity map
    clEnqueueNDRangeKernel(queue, zncc_kernel, 2, NULL, global, NULL, 0, NULL, NULL);

    // Compute right disparity map
    clSetKernelArg(zncc_kernel, 0, sizeof(cl_mem), &d_right_gray);
    clSetKernelArg(zncc_kernel, 1, sizeof(cl_mem), &d_left_gray);
    clSetKernelArg(zncc_kernel, 2, sizeof(cl_mem), &d_disp_right);
    clEnqueueNDRangeKernel(queue, zncc_kernel, 2, NULL, global, NULL, 0, NULL, NULL);

    // Set up cross-check kernel arguments
    clSetKernelArg(crosscheck_kernel, 0, sizeof(cl_mem), &d_disp_left);
    clSetKernelArg(crosscheck_kernel, 1, sizeof(cl_mem), &d_disp_right);
    clSetKernelArg(crosscheck_kernel, 2, sizeof(cl_mem), &d_disp_final);
    clSetKernelArg(crosscheck_kernel, 3, sizeof(int), &WIDTH);
    clSetKernelArg(crosscheck_kernel, 4, sizeof(int), &threshold);

    // Perform cross-check
    clEnqueueNDRangeKernel(queue, crosscheck_kernel, 2, NULL, global, NULL, 0, NULL, NULL);

    // Download results
    disp_final = (float*)malloc(WIDTH*HEIGHT*sizeof(float));
    clEnqueueReadBuffer(queue, d_disp_final, CL_TRUE, 0, WIDTH*HEIGHT*sizeof(float), disp_final, 0, NULL, NULL);

    // Occlusion filling (CPU)
    for(int y = 0; y < HEIGHT; y++) {
        float last_valid = 0;
        for(int x = 0; x < WIDTH; x++) {
            if(disp_final[y*WIDTH+x] == 0) {
                disp_final[y*WIDTH+x] = last_valid;
            } else {
                last_valid = disp_final[y*WIDTH+x];
            }
        }
    }

    // Save output
    unsigned char *output = (unsigned char*)malloc(WIDTH*HEIGHT);
    for(int i = 0; i < WIDTH*HEIGHT; i++) {
        output[i] = (unsigned char)((disp_final[i] / MAX_DISP) * 255);
    }
    lodepng_encode_file("depthmap.png", output, WIDTH, HEIGHT, LCT_GREY, 8);

    // Cleanup
    free(left_png);
    free(right_png);
    free(disp_final);
    free(output);
    clReleaseMemObject(d_left);
    clReleaseMemObject(d_right);
    clReleaseMemObject(d_left_gray);
    clReleaseMemObject(d_right_gray);
    clReleaseMemObject(d_disp_left);
    clReleaseMemObject(d_disp_right);
    clReleaseMemObject(d_disp_final);
}

int main() {
    setup_opencl();
    run_gpu_pipeline();
    cleanup();
    return 0;
}