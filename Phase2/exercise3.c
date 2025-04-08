// image_processing_cl.c
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <CL/cl.h>
#include "lodepng.h"

#define MAX_SOURCE_SIZE 16384
#define FILTER_SIZE 5
#define DOWNSCALE_FACTOR 4

// OpenCL kernel sources
const char* resize_kernel_source =
"__kernel void resize(__global uchar4* input, __global uchar4* output,"
"                    uint width, uint new_width, uint new_height) {"
"    uint x = get_global_id(0);"
"    uint y = get_global_id(1);"
"    output[y*new_width + x] = input[(y*4)*width + (x*4)];"
"}";

const char* grayscale_kernel_source =
"__kernel void grayscale(__global uchar4* input, __global uchar* output) {"
"    uint idx = get_global_id(0);"
"    uchar4 pixel = input[idx];"
"    output[idx] = 0.2126f*pixel.x + 0.7152f*pixel.y + 0.0722f*pixel.z;"
"}";

const char* filter_kernel_source =
"__kernel void filter(__global uchar* input, __global uchar* output,"
"                     uint width, uint height) {"
"    int x = get_global_id(0);"
"    int y = get_global_id(1);"
"    int border = 2;" // Changed variable name from 'half' to 'border'
"    if(x < border || x >= width - border || y < border || y >= height - border) return;"
"    int sum = 0;"
"    for(int fy=-2; fy<=2; fy++) {"
"        for(int fx=-2; fx<=2; fx++) {"
"            sum += input[(y+fy)*width + (x+fx)];"
"        }"
"    }"
"    output[y*width + x] = sum / 25;"
"}";

// Helper function to create context/queue
cl_context create_context(cl_device_id *device) {
    cl_platform_id platform;
    clGetPlatformIDs(1, &platform, NULL);
    clGetDeviceIDs(platform, CL_DEVICE_TYPE_GPU, 1, device, NULL);
    
    cl_context context = clCreateContext(NULL, 1, device, NULL, NULL, NULL);
    return context;
}

int main() {
    // Read image using lodepng (same as C version)
    unsigned char* image;
    unsigned width, height;
    lodepng_decode32_file(&image, &width, &height, "im0.png");
    
    // Create OpenCL resources
    cl_device_id device;
    cl_context context = create_context(&device);
    cl_queue_properties properties[] = { CL_QUEUE_PROPERTIES, CL_QUEUE_PROFILING_ENABLE, 0 };
    cl_command_queue queue = clCreateCommandQueueWithProperties(context, device, properties, NULL);
    
    // Create programs
    cl_program resize_program = clCreateProgramWithSource(context, 1, &resize_kernel_source, NULL, NULL);
    cl_program gray_program = clCreateProgramWithSource(context, 1, &grayscale_kernel_source, NULL, NULL);
    cl_program filter_program = clCreateProgramWithSource(context, 1, &filter_kernel_source, NULL, NULL);
    
    clBuildProgram(resize_program, 1, &device, NULL, NULL, NULL);
    clBuildProgram(gray_program, 1, &device, NULL, NULL, NULL);
    clBuildProgram(filter_program, 1, &device, NULL, NULL, NULL);
    
    // Create kernels
    cl_kernel resize_kernel = clCreateKernel(resize_program, "resize", NULL);
    cl_kernel gray_kernel = clCreateKernel(gray_program, "grayscale", NULL);
    cl_kernel filter_kernel = clCreateKernel(filter_program, "filter", NULL);
    
    // Create buffers
    cl_uint  new_width = width/DOWNSCALE_FACTOR;
    cl_uint  new_height = height/DOWNSCALE_FACTOR;
    
    cl_mem input_buff = clCreateBuffer(context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, 
                                      width*height*4, image, NULL);
    cl_mem resized_buff = clCreateBuffer(context, CL_MEM_READ_WRITE, new_width*new_height*4, NULL, NULL);
    cl_mem gray_buff = clCreateBuffer(context, CL_MEM_READ_WRITE, new_width*new_height, NULL, NULL);
    cl_mem filtered_buff = clCreateBuffer(context, CL_MEM_READ_WRITE, new_width*new_height, NULL, NULL);
    
    // Execute resize kernel
    clSetKernelArg(resize_kernel, 0, sizeof(cl_mem), &input_buff);
    clSetKernelArg(resize_kernel, 1, sizeof(cl_mem), &resized_buff);
    clSetKernelArg(resize_kernel, 2, sizeof(unsigned), &width);
    clSetKernelArg(resize_kernel, 3, sizeof(cl_uint), &new_width);
    clSetKernelArg(resize_kernel, 4, sizeof(cl_uint), &new_height);
    
    size_t global_work[2] = {new_width, new_height}; //{735, 505}
    cl_int err = clEnqueueNDRangeKernel(queue, resize_kernel, 2, NULL, global_work, NULL, 0, NULL, NULL);
    printf("Line: %d - error: %d\n", __LINE__, err);
    // Execute grayscale kernel
    clSetKernelArg(gray_kernel, 0, sizeof(cl_mem), &resized_buff);
    clSetKernelArg(gray_kernel, 1, sizeof(cl_mem), &gray_buff);
    
    size_t gray_global = new_width*new_height;
    err = clEnqueueNDRangeKernel(queue, gray_kernel, 1, NULL, &gray_global, NULL, 0, NULL, NULL);
    printf("Line: %d - error: %d\n", __LINE__, err);
    //check if grayscale image data exist.
    unsigned char* gray_img = (unsigned char*)malloc(new_width*new_height);
    err = clEnqueueReadBuffer(queue, gray_buff, CL_TRUE, 0, new_width*new_height, gray_img, 0, NULL, NULL);
    printf("Line: %d - error: %d\n", __LINE__, err);
    printf("gray scale image: %d\n", gray_img[0]);
    //save the grayscale image
    lodepng_encode_file("gray_cl.png", gray_img, new_width, new_height, LCT_GREY, 8);

    // Execute filter kernel
    clSetKernelArg(filter_kernel, 0, sizeof(cl_mem), &gray_buff);
    clSetKernelArg(filter_kernel, 1, sizeof(cl_mem), &filtered_buff);
    clSetKernelArg(filter_kernel, 2, sizeof(cl_uint), &new_width);
    clSetKernelArg(filter_kernel, 3, sizeof(cl_uint), &new_height);
    
    err = clEnqueueNDRangeKernel(queue, filter_kernel, 2, NULL, global_work, NULL, 0, NULL, NULL);
    printf("Line: %d - error: %d\n", __LINE__, err);

    // Read result
    unsigned char* filtered = (unsigned char*)malloc(new_width*new_height);
    err = clEnqueueReadBuffer(queue, filtered_buff, CL_TRUE, 0, new_width*new_height, filtered, 0, NULL, NULL);
    printf("Line: %d - error: %d\n", __LINE__, err);
    printf("Check if filtered array has data:\n");
    // Print the array as a 10x10 matrix
    for (int row = 0; row < 20; row++) {
        for (int col = 0; col < 20; col++) {
            // Calculate the index in the 1D array
            int index = row * new_width + col;
            printf("%3d ", filtered[index]);
        }
        // Print a new line at the end of each row
        printf("\n");
    }
    
    printf("data at index 1: %d\n", filtered[0]);

    // Save result
    lodepng_encode_file("output_cl.png", filtered, new_width, new_height, LCT_GREY, 8);

    // Cleanup
    clReleaseMemObject(input_buff);
    clReleaseMemObject(resized_buff);
    clReleaseMemObject(gray_buff);
    clReleaseMemObject(filtered_buff);
    clReleaseKernel(resize_kernel);
    clReleaseKernel(gray_kernel);
    clReleaseKernel(filter_kernel);
    clReleaseProgram(resize_program);
    clReleaseProgram(gray_program);
    clReleaseProgram(filter_program);
    clReleaseCommandQueue(queue);
    clReleaseContext(context);
    free(image);
    free(gray_img);
    free(filtered);
    
    return 0;
}