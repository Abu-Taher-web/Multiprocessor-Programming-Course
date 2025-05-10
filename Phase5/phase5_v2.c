#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include <CL/cl.h>
#include "lodepng.h"

#define MAX_DISP_VAL 65
#define WINDOW_SIZE_VAL 4
#define THRESHOLD_VAL 8
#define FILTER_WINDOW_VAL 2

const unsigned ORIG_WIDTH = 2940;
const unsigned ORIG_HEIGHT = 2016;
const unsigned WIDTH = 735;
const unsigned HEIGHT = 504;

typedef struct {
    const char* name;
    double time;
} TimingProfile;

#define MAX_PROFILE_ENTRIES 20
TimingProfile timings[MAX_PROFILE_ENTRIES];
int profile_count = 0;

// OpenCL globals
cl_platform_id platform;
cl_device_id device;
cl_context context;
cl_command_queue queue;
cl_program program;

const char* kernel_source = 
"#define WIDTH %d\n"
"#define HEIGHT %d\n"
"#define MAX_DISP %d\n"
"#define WINDOW_SIZE %d\n"
"#define THRESHOLD %d\n"
"#define FILTER_WINDOW %d\n\n"
"__kernel void resize_rgba(__global uchar4* input, __global uchar4* output, uint orig_width, uint orig_height) {\n"
"    uint x = get_global_id(0);\n"
"    uint y = get_global_id(1);\n"
"    if (x >= WIDTH || y >= HEIGHT) return;\n"
"    uint src_x = x * (orig_width / WIDTH);\n"
"    uint src_y = y * (orig_height / HEIGHT);\n"
"    output[y*WIDTH + x] = input[src_y*orig_width + src_x];\n"
"}\n\n"
"__kernel void rgba_to_gray(__global uchar4* rgba, __global uchar* gray) {\n"
"    uint x = get_global_id(0);\n"
"    uint y = get_global_id(1);\n"
"    uchar4 pixel = rgba[y*WIDTH + x];\n"
"    gray[y*WIDTH + x] = (uchar)(0.2126f*pixel.x + 0.7152f*pixel.y + 0.0722f*pixel.z);\n"
"}\n\n"
"__kernel void mean_kernel(__global const uchar* image, __global float* mean, int window_size) {\n"
"    int x = get_global_id(0);\n"
"    int y = get_global_id(1);\n"
"    if (x >= WIDTH || y >= HEIGHT) return;\n"
"    float sum = 0.0f;\n"
"    int count = 0;\n"
"    for (int dy = -window_size; dy <= window_size; dy++) {\n"
"        for (int dx = -window_size; dx <= window_size; dx++) {\n"
"            int nx = x + dx;\n"
"            int ny = y + dy;\n"
"            if (nx >= 0 && nx < WIDTH && ny >= 0 && ny < HEIGHT) {\n"
"                sum += convert_float(image[ny*WIDTH + nx]);\n"
"                count++;\n"
"            }\n"
"        }\n"
"    }\n"
"    mean[y*WIDTH + x] = sum / count;\n"
"}\n\n"
"__kernel void zncc_left_right(__global const uchar* left, __global const uchar* right,\n"
"                             __global const float* left_mean, __global const float* right_mean,\n"
"                             __global float* disp_map, int window_size) {\n"
"    int x = get_global_id(0);\n"
"    int y = get_global_id(1);\n"
"    float max_zncc = -INFINITY;\n"
"    int best_d = 0;\n"
"    for (int d = 0; d < MAX_DISP; d++) {\n"
"        if (x - d < 0) continue;\n"
"        float mean_l = left_mean[y*WIDTH + x];\n"
"        float mean_r = right_mean[y*WIDTH + (x-d)];\n"
"        float numerator = 0.0f, denom_l = 0.0f, denom_r = 0.0f;\n"
"        for (int dy = -window_size; dy <= window_size; dy++) {\n"
"            for (int dx = -window_size; dx <= window_size; dx++) {\n"
"                int nx = x + dx;\n"
"                int ny = y + dy;\n"
"                int nx_d = nx - d;\n"
"                if (nx >= 0 && nx < WIDTH && ny >= 0 && ny < HEIGHT && nx_d >= 0 && nx_d < WIDTH) {\n"
"                    float l_val = convert_float(left[ny*WIDTH + nx]) - mean_l;\n"
"                    float r_val = convert_float(right[ny*WIDTH + nx_d]) - mean_r;\n"
"                    numerator += l_val * r_val;\n"
"                    denom_l += l_val * l_val;\n"
"                    denom_r += r_val * r_val;\n"
"                }\n"
"            }\n"
"        }\n"
"        float denominator = sqrt(denom_l * denom_r);\n"
"        float zncc = (denominator != 0.0f) ? (numerator / denominator) : 0.0f;\n"
"        if (zncc > max_zncc) { max_zncc = zncc; best_d = d; }\n"
"    }\n"
"    disp_map[y*WIDTH + x] = (float)best_d;\n"
"}\n\n"
"__kernel void cross_check(__global const float* disp_left, __global const float* disp_right,\n"
"                         __global float* disp_final) {\n"
"    int x = get_global_id(0);\n"
"    int y = get_global_id(1);\n"
"    int d = (int)disp_left[y*WIDTH + x];\n"
"    if (x - d >= 0) {\n"
"        float right_disp = disp_right[y*WIDTH + (x-d)];\n"
"        disp_final[y*WIDTH + x] = (fabs(d - right_disp) > THRESHOLD) ? 0.0f : d;\n"
"    } else {\n"
"        disp_final[y*WIDTH + x] = 0.0f;\n"
"    }\n"
"}\n\n"
"__kernel void occlusion_horizontal(__global float* disp) {\n"
"    int y = get_global_id(0);\n"
"    float last_valid = 0.0f;\n"
"    for (int x = 0; x < WIDTH; x++) {\n"
"        if (disp[y*WIDTH + x] != 0.0f) last_valid = disp[y*WIDTH + x];\n"
"        else disp[y*WIDTH + x] = last_valid;\n"
"    }\n"
"    last_valid = 0.0f;\n"
"    for (int x = WIDTH-1; x >= 0; x--) {\n"
"        if (disp[y*WIDTH + x] != 0.0f) last_valid = disp[y*WIDTH + x];\n"
"        else disp[y*WIDTH + x] = last_valid;\n"
"    }\n"
"}\n\n"
"__kernel void weighted_median_filter(__global float* input, __global float* output) {\n"
"    int x = get_global_id(0);\n"
"    int y = get_global_id(1);\n"
"    if (x >= WIDTH || y >= HEIGHT) return;\n"
"    \n"
"    __local float window[25];\n"
"    __local float weights[25];\n"
"    int count = 0;\n"
"    float total_weight = 0.0f;\n"
"    \n"
"    for (int dy = -FILTER_WINDOW; dy <= FILTER_WINDOW; dy++) {\n"
"        for (int dx = -FILTER_WINDOW; dx <= FILTER_WINDOW; dx++) {\n"
"            int nx = x + dx;\n"
"            int ny = y + dy;\n"
"            if (nx >= 0 && nx < WIDTH && ny >= 0 && ny < HEIGHT) {\n"
"                window[count] = input[ny*WIDTH + nx];\n"
"                weights[count] = 1.0f / (1 + abs(dx) + abs(dy));\n"
"                total_weight += weights[count];\n"
"                count++;\n"
"            }\n"
"        }\n"
"    }\n"
"    \n"
"    for (int i = 0; i < count-1; i++) {\n"
"        for (int j = i+1; j < count; j++) {\n"
"            if (window[j] < window[i]) {\n"
"                float temp = window[i];\n"
"                window[i] = window[j];\n"
"                window[j] = temp;\n"
"                temp = weights[i];\n"
"                weights[i] = weights[j];\n"
"                weights[j] = temp;\n"
"            }\n"
"        }\n"
"    }\n"
"    \n"
"    float half_weight = total_weight / 2.0f;\n"
"    float current_weight = 0.0f;\n"
"    int median_index = 0;\n"
"    while (current_weight < half_weight && median_index < count) {\n"
"        current_weight += weights[median_index++];\n"
"    }\n"
"    output[y*WIDTH + x] = (median_index > 0) ? window[median_index-1] : 0.0f;\n"
"}\n";

void start_timer(const char* name) {
    timings[profile_count].name = name;
    timings[profile_count].time = clock();
}

void end_timer() {
    timings[profile_count].time = (double)(clock() - timings[profile_count].time) / CLOCKS_PER_SEC;
    profile_count++;
}

void print_timings() {
    printf("\n--- Performance Profile ---\n");
    for(int i = 0; i < profile_count; i++) {
        printf("%-25s: %7.3f s\n", timings[i].name, timings[i].time);
    }
}

void print_device_info(cl_device_id device) {
    char name[128];
    clGetDeviceInfo(device, CL_DEVICE_NAME, sizeof(name), name, NULL);
    printf("Using OpenCL device: %s\n", name);
}

cl_program build_program(cl_context context, cl_device_id device) {
    char full_source[8192];
    snprintf(full_source, sizeof(full_source), kernel_source,
        WIDTH, HEIGHT, MAX_DISP_VAL, WINDOW_SIZE_VAL, THRESHOLD_VAL, FILTER_WINDOW_VAL);

    cl_int err;
    size_t source_len = strlen(full_source);
    cl_program program = clCreateProgramWithSource(context, 1, (const char**)&full_source, &source_len, &err);
    
    const char* options = "-cl-mad-enable -cl-fast-relaxed-math";
    err = clBuildProgram(program, 1, &device, options, NULL, NULL);
    
    if (err != CL_SUCCESS) {
        size_t log_size;
        clGetProgramBuildInfo(program, device, CL_PROGRAM_BUILD_LOG, 0, NULL, &log_size);
        char* log = (char*)malloc(log_size);
        clGetProgramBuildInfo(program, device, CL_PROGRAM_BUILD_LOG, log_size, log, NULL);
        printf("Build failed:\n%s\n", log);
        free(log);
        exit(1);
    }
    return program;
}

unsigned char* load_image(const char *filename) {
    unsigned error;
    unsigned char *png;
    unsigned width, height;
    
    error = lodepng_decode32_file(&png, &width, &height, filename);
    if (error) {
        printf("Error %u: %s\n", error, lodepng_error_text(error));
        exit(1);
    }
    return png;
}

void save_float_image(const char *filename, float *data) {
    unsigned char *img = (unsigned char*)malloc(WIDTH * HEIGHT);
    for(int i = 0; i < WIDTH*HEIGHT; i++) {
        img[i] = (unsigned char)fmin(255, data[i] * 255.0f / MAX_DISP_VAL);
    }
    lodepng_encode_file(filename, img, WIDTH, HEIGHT, LCT_GREY, 8);
    free(img);
}

int main() {
    start_timer("Total Execution");
    
    // Initialize OpenCL
    cl_int err;
    clGetPlatformIDs(1, &platform, NULL);
    clGetDeviceIDs(platform, CL_DEVICE_TYPE_GPU, 1, &device, NULL);
    context = clCreateContext(NULL, 1, &device, NULL, NULL, &err);
    
    cl_queue_properties props[] = {CL_QUEUE_PROPERTIES, CL_QUEUE_PROFILING_ENABLE, 0};
    queue = clCreateCommandQueueWithProperties(context, device, props, &err);
    
    print_device_info(device);
    program = build_program(context, device);

    // Load images
    start_timer("Load Images");
    unsigned char *left_orig = load_image("left.png");
    unsigned char *right_orig = load_image("right.png");
    end_timer();

    // Create device buffers
    cl_mem d_left_orig = clCreateBuffer(context, CL_MEM_READ_ONLY, ORIG_WIDTH*ORIG_HEIGHT*4, NULL, &err);
    cl_mem d_right_orig = clCreateBuffer(context, CL_MEM_READ_ONLY, ORIG_WIDTH*ORIG_HEIGHT*4, NULL, &err);
    cl_mem d_left_resized = clCreateBuffer(context, CL_MEM_READ_WRITE, WIDTH*HEIGHT*4, NULL, &err);
    cl_mem d_right_resized = clCreateBuffer(context, CL_MEM_READ_WRITE, WIDTH*HEIGHT*4, NULL, &err);
    cl_mem d_left_gray = clCreateBuffer(context, CL_MEM_READ_WRITE, WIDTH*HEIGHT, NULL, &err);
    cl_mem d_right_gray = clCreateBuffer(context, CL_MEM_READ_WRITE, WIDTH*HEIGHT, NULL, &err);

    // Upload original images
    start_timer("Upload Original Images");
    clEnqueueWriteBuffer(queue, d_left_orig, CL_TRUE, 0, ORIG_WIDTH*ORIG_HEIGHT*4, left_orig, 0, NULL, NULL);
    clEnqueueWriteBuffer(queue, d_right_orig, CL_TRUE, 0, ORIG_WIDTH*ORIG_HEIGHT*4, right_orig, 0, NULL, NULL);
    end_timer();

    // Resize images
    start_timer("Resize Images");
    cl_kernel resize_kernel = clCreateKernel(program, "resize_rgba", &err);
    size_t resize_global[2] = {WIDTH, HEIGHT};
    
    clSetKernelArg(resize_kernel, 0, sizeof(cl_mem), &d_left_orig);
    clSetKernelArg(resize_kernel, 1, sizeof(cl_mem), &d_left_resized);
    clSetKernelArg(resize_kernel, 2, sizeof(unsigned int), &ORIG_WIDTH);
    clSetKernelArg(resize_kernel, 3, sizeof(unsigned int), &ORIG_HEIGHT);
    clEnqueueNDRangeKernel(queue, resize_kernel, 2, NULL, resize_global, NULL, 0, NULL, NULL);
    
    clSetKernelArg(resize_kernel, 0, sizeof(cl_mem), &d_right_orig);
    clSetKernelArg(resize_kernel, 1, sizeof(cl_mem), &d_right_resized);
    clEnqueueNDRangeKernel(queue, resize_kernel, 2, NULL, resize_global, NULL, 0, NULL, NULL);
    clFinish(queue);
    end_timer();

    // Convert to grayscale
    start_timer("Grayscale Conversion");
    cl_kernel gray_kernel = clCreateKernel(program, "rgba_to_gray", &err);
    
    clSetKernelArg(gray_kernel, 0, sizeof(cl_mem), &d_left_resized);
    clSetKernelArg(gray_kernel, 1, sizeof(cl_mem), &d_left_gray);
    clEnqueueNDRangeKernel(queue, gray_kernel, 2, NULL, resize_global, NULL, 0, NULL, NULL);
    
    clSetKernelArg(gray_kernel, 0, sizeof(cl_mem), &d_right_resized);
    clSetKernelArg(gray_kernel, 1, sizeof(cl_mem), &d_right_gray);
    clEnqueueNDRangeKernel(queue, gray_kernel, 2, NULL, resize_global, NULL, 0, NULL, NULL);
    clFinish(queue);
    end_timer();

    // Compute mean images
    start_timer("Mean Calculation");
    cl_mem d_left_mean = clCreateBuffer(context, CL_MEM_READ_WRITE, WIDTH*HEIGHT*sizeof(float), NULL, &err);
    cl_mem d_right_mean = clCreateBuffer(context, CL_MEM_READ_WRITE, WIDTH*HEIGHT*sizeof(float), NULL, &err);
    cl_kernel mean_kernel = clCreateKernel(program, "mean_kernel", &err);
    int window_size = WINDOW_SIZE_VAL;
    
    clSetKernelArg(mean_kernel, 0, sizeof(cl_mem), &d_left_gray);
    clSetKernelArg(mean_kernel, 1, sizeof(cl_mem), &d_left_mean);
    clSetKernelArg(mean_kernel, 2, sizeof(int), &window_size);
    clEnqueueNDRangeKernel(queue, mean_kernel, 2, NULL, resize_global, NULL, 0, NULL, NULL);
    
    clSetKernelArg(mean_kernel, 0, sizeof(cl_mem), &d_right_gray);
    clSetKernelArg(mean_kernel, 1, sizeof(cl_mem), &d_right_mean);
    clEnqueueNDRangeKernel(queue, mean_kernel, 2, NULL, resize_global, NULL, 0, NULL, NULL);
    clFinish(queue);
    end_timer();

    // Compute disparities
    start_timer("Disparity Calculation");
    cl_mem d_disp_left = clCreateBuffer(context, CL_MEM_READ_WRITE, WIDTH*HEIGHT*sizeof(float), NULL, &err);
    cl_mem d_disp_right = clCreateBuffer(context, CL_MEM_READ_WRITE, WIDTH*HEIGHT*sizeof(float), NULL, &err);
    cl_kernel zncc_kernel = clCreateKernel(program, "zncc_left_right", &err);
    
    // Left to Right
    clSetKernelArg(zncc_kernel, 0, sizeof(cl_mem), &d_left_gray);
    clSetKernelArg(zncc_kernel, 1, sizeof(cl_mem), &d_right_gray);
    clSetKernelArg(zncc_kernel, 2, sizeof(cl_mem), &d_left_mean);
    clSetKernelArg(zncc_kernel, 3, sizeof(cl_mem), &d_right_mean);
    clSetKernelArg(zncc_kernel, 4, sizeof(cl_mem), &d_disp_left);
    clSetKernelArg(zncc_kernel, 5, sizeof(int), &window_size);
    clEnqueueNDRangeKernel(queue, zncc_kernel, 2, NULL, resize_global, NULL, 0, NULL, NULL);
    
    // Right to Left
    clSetKernelArg(zncc_kernel, 0, sizeof(cl_mem), &d_right_gray);
    clSetKernelArg(zncc_kernel, 1, sizeof(cl_mem), &d_left_gray);
    clSetKernelArg(zncc_kernel, 2, sizeof(cl_mem), &d_right_mean);
    clSetKernelArg(zncc_kernel, 3, sizeof(cl_mem), &d_left_mean);
    clSetKernelArg(zncc_kernel, 4, sizeof(cl_mem), &d_disp_right);
    clEnqueueNDRangeKernel(queue, zncc_kernel, 2, NULL, resize_global, NULL, 0, NULL, NULL);
    clFinish(queue);
    end_timer();

    // Cross-check
    start_timer("Cross Check");
    cl_mem d_disp_cross = clCreateBuffer(context, CL_MEM_READ_WRITE, WIDTH*HEIGHT*sizeof(float), NULL, &err);
    cl_kernel cross_kernel = clCreateKernel(program, "cross_check", &err);
    
    clSetKernelArg(cross_kernel, 0, sizeof(cl_mem), &d_disp_left);
    clSetKernelArg(cross_kernel, 1, sizeof(cl_mem), &d_disp_right);
    clSetKernelArg(cross_kernel, 2, sizeof(cl_mem), &d_disp_cross);
    clEnqueueNDRangeKernel(queue, cross_kernel, 2, NULL, resize_global, NULL, 0, NULL, NULL);
    clFinish(queue);
    end_timer();

    // Occlusion filling
    start_timer("Occlusion Filling");
    cl_kernel occlusion_kernel = clCreateKernel(program, "occlusion_horizontal", &err);
    size_t occlusion_global[1] = {HEIGHT};
    clEnqueueNDRangeKernel(queue, occlusion_kernel, 1, NULL, occlusion_global, NULL, 0, NULL, NULL);
    clFinish(queue);
    end_timer();

    // Post-processing
    start_timer("Post-processing");
    cl_mem d_disp_filtered = clCreateBuffer(context, CL_MEM_READ_WRITE, WIDTH*HEIGHT*sizeof(float), NULL, &err);
    cl_kernel median_kernel = clCreateKernel(program, "weighted_median_filter", &err);
    
    clSetKernelArg(median_kernel, 0, sizeof(cl_mem), &d_disp_cross);
    clSetKernelArg(median_kernel, 1, sizeof(cl_mem), &d_disp_filtered);
    clEnqueueNDRangeKernel(queue, median_kernel, 2, NULL, resize_global, NULL, 0, NULL, NULL);
    clFinish(queue);
    end_timer();

    // Download final disparity
    start_timer("Download Results");
    float *final_disp = (float*)malloc(WIDTH*HEIGHT*sizeof(float));
    clEnqueueReadBuffer(queue, d_disp_filtered, CL_TRUE, 0, WIDTH*HEIGHT*sizeof(float), final_disp, 0, NULL, NULL);
    end_timer();

    // Save results
    save_float_image("disparity.png", final_disp);

    // Cleanup
    clReleaseMemObject(d_left_orig);
    clReleaseMemObject(d_right_orig);
    clReleaseMemObject(d_left_resized);
    clReleaseMemObject(d_right_resized);
    clReleaseMemObject(d_left_gray);
    clReleaseMemObject(d_right_gray);
    clReleaseMemObject(d_left_mean);
    clReleaseMemObject(d_right_mean);
    clReleaseMemObject(d_disp_left);
    clReleaseMemObject(d_disp_right);
    clReleaseMemObject(d_disp_cross);
    clReleaseMemObject(d_disp_filtered);
    
    clReleaseKernel(resize_kernel);
    clReleaseKernel(gray_kernel);
    clReleaseKernel(mean_kernel);
    clReleaseKernel(zncc_kernel);
    clReleaseKernel(cross_kernel);
    clReleaseKernel(occlusion_kernel);
    clReleaseKernel(median_kernel);
    
    clReleaseProgram(program);
    clReleaseCommandQueue(queue);
    clReleaseContext(context);

    free(left_orig);
    free(right_orig);
    free(final_disp);

    end_timer();
    print_timings();
    return 0;
}