#include <stdio.h>
#include <stdlib.h>
#include <CL/cl.h>
#include "lodepng.h"

const unsigned MAX_DISP = 65;
const unsigned WINDOW_SIZE = 4;
const unsigned THRESHOLD = 2;
const unsigned MAX_SEARCH_RADIUS = 2;

const unsigned ORIG_WIDTH = 2940;
const unsigned ORIG_HEIGHT = 2016;
const unsigned WIDTH = 735;
const unsigned HEIGHT = 504;


void print_platform_and_device_info() {
    cl_uint num_platforms;
    clGetPlatformIDs(0, NULL, &num_platforms);
    cl_platform_id *platforms = (cl_platform_id*)malloc(num_platforms * sizeof(cl_platform_id));
    clGetPlatformIDs(num_platforms, platforms, NULL);

    for (cl_uint i = 0; i < num_platforms; i++) {
        char platform_name[128];
        clGetPlatformInfo(platforms[i], CL_PLATFORM_NAME, 128, platform_name, NULL);
        printf("Platform %u: %s\n", i, platform_name);
        char platform_info[1024];
        
        // Platform
        clGetPlatformInfo(platforms[i], CL_PLATFORM_NAME, sizeof(platform_info), platform_info, NULL);
        printf("=== Platform %u ===\nName: %s\n", i, platform_info);
        
        clGetPlatformInfo(platforms[i], CL_PLATFORM_VENDOR, sizeof(platform_info), platform_info, NULL);
        printf("Vendor: %s\n", platform_info);
        
        clGetPlatformInfo(platforms[i], CL_PLATFORM_VERSION, sizeof(platform_info), platform_info, NULL);
        printf("Version: %s\n", platform_info);

        cl_uint num_devices;
        clGetDeviceIDs(platforms[i], CL_DEVICE_TYPE_ALL, 0, NULL, &num_devices);
        cl_device_id *devices = (cl_device_id*)malloc(num_devices * sizeof(cl_device_id));
        clGetDeviceIDs(platforms[i], CL_DEVICE_TYPE_ALL, num_devices, devices, NULL);

        for (cl_uint j = 0; j < num_devices; j++) {
            cl_device_id device = devices[j];
            char device_name[128];
            clGetDeviceInfo(device, CL_DEVICE_NAME, 128, device_name, NULL);
            printf("  Device %u: %s\n", j, device_name);

            cl_device_local_mem_type local_mem_type;
            clGetDeviceInfo(device, CL_DEVICE_LOCAL_MEM_TYPE, sizeof(local_mem_type), &local_mem_type, NULL);
            printf("    CL_DEVICE_LOCAL_MEM_TYPE: %s\n", local_mem_type == CL_LOCAL ? "CL_LOCAL" : "CL_GLOBAL");

            cl_ulong local_mem_size;
            clGetDeviceInfo(device, CL_DEVICE_LOCAL_MEM_SIZE, sizeof(local_mem_size), &local_mem_size, NULL);
            printf("    CL_DEVICE_LOCAL_MEM_SIZE: %lu KB\n", local_mem_size / 1024);

            cl_uint compute_units;
            clGetDeviceInfo(device, CL_DEVICE_MAX_COMPUTE_UNITS, sizeof(compute_units), &compute_units, NULL);
            printf("    CL_DEVICE_MAX_COMPUTE_UNITS: %u\n", compute_units);

            cl_uint clock_freq;
            clGetDeviceInfo(device, CL_DEVICE_MAX_CLOCK_FREQUENCY, sizeof(clock_freq), &clock_freq, NULL);
            printf("    CL_DEVICE_MAX_CLOCK_FREQUENCY: %u MHz\n", clock_freq);

            cl_ulong const_buffer_size;
            clGetDeviceInfo(device, CL_DEVICE_MAX_CONSTANT_BUFFER_SIZE, sizeof(const_buffer_size), &const_buffer_size, NULL);
            printf("    CL_DEVICE_MAX_CONSTANT_BUFFER_SIZE: %lu KB\n", const_buffer_size / 1024);

            size_t max_work_group_size;
            clGetDeviceInfo(device, CL_DEVICE_MAX_WORK_GROUP_SIZE, sizeof(max_work_group_size), &max_work_group_size, NULL);
            printf("    CL_DEVICE_MAX_WORK_GROUP_SIZE: %zu\n", max_work_group_size);

            size_t max_work_item_sizes[3];
            clGetDeviceInfo(device, CL_DEVICE_MAX_WORK_ITEM_SIZES, sizeof(max_work_item_sizes), max_work_item_sizes, NULL);
            printf("    CL_DEVICE_MAX_WORK_ITEM_SIZES: %zu, %zu, %zu\n", max_work_item_sizes[0], max_work_item_sizes[1], max_work_item_sizes[2]);
        }
        free(devices);
    }
    free(platforms);
}


// Add profiling function
void print_profiling_info(const char* name, cl_event event) {
    cl_ulong start, end;
    clGetEventProfilingInfo(event, CL_PROFILING_COMMAND_START, sizeof(start), &start, NULL);
    clGetEventProfilingInfo(event, CL_PROFILING_COMMAND_END, sizeof(end), &end, NULL);
    double elapsed = (end - start) * 1e-6; // Convert to milliseconds
    printf("%s time: %.3f ms\n", name, elapsed);
}

cl_device_id create_device() {
    cl_platform_id platform;
    cl_device_id dev;
    int err;

    err = clGetPlatformIDs(1, &platform, NULL);
    if (err < 0) {
        perror("Couldn't identify a platform");
        exit(1);
    }

    err = clGetDeviceIDs(platform, CL_DEVICE_TYPE_GPU, 1, &dev, NULL);
    if (err == CL_DEVICE_NOT_FOUND)
        err = clGetDeviceIDs(platform, CL_DEVICE_TYPE_CPU, 1, &dev, NULL);
    if (err < 0) {
        perror("Couldn't access any devices");
        exit(1);
    }
    return dev;
}

cl_program build_program(cl_context ctx, cl_device_id dev, const char* filename, const char* options) {
    FILE *fp = fopen(filename, "rb");
    if (!fp) {
        perror("Couldn't open kernel file");
        exit(1);
    }
    fseek(fp, 0, SEEK_END);
    size_t size = ftell(fp);
    rewind(fp);
    char *source = (char*)malloc(size + 1);
    fread(source, 1, size, fp);
    fclose(fp);
    source[size] = '\0';

    cl_program program = clCreateProgramWithSource(ctx, 1, (const char**)&source, &size, NULL);
    free(source);

    cl_int err = clBuildProgram(program, 1, &dev, options, NULL, NULL);
    if (err < 0) {
        size_t log_size;
        clGetProgramBuildInfo(program, dev, CL_PROGRAM_BUILD_LOG, 0, NULL, &log_size);
        char *log = (char*)malloc(log_size);
        clGetProgramBuildInfo(program, dev, CL_PROGRAM_BUILD_LOG, log_size, log, NULL);
        printf("Build error:\n%s\n", log);
        free(log);
        exit(1);
    }
    return program;
}




int main(){

    /*..........Get the DEVICE information................*/
    // Print device information
    print_platform_and_device_info();




    /*...............Load images..........................*/
    // Load input images
    unsigned char *im0_data = NULL, *im1_data = NULL;
    unsigned l_width, l_height, r_width, r_height;

    unsigned error = lodepng_decode32_file(&im0_data, &l_width, &l_height, "im0.png");
    if(error) printf("Error %u: %s\n", error, lodepng_error_text(error));

    error = lodepng_decode32_file(&im1_data, &r_width, &r_height, "im1.png");
    if(error) printf("Error %u: %s\n", error, lodepng_error_text(error));

    //Check if your image loaded correctly
    printf("Loaded left image: %ux%u\n", l_width, l_height);
    printf("Loaded right image: %ux%u\n", r_width, r_height);
    if(l_width != ORIG_WIDTH || l_height != ORIG_HEIGHT) {
        printf("Error: Left image has incorrect dimensions!\n");
        exit(1);
    }


    /*................Prepare the DEVICE for input.....................*/
    // Create device, context and queues
    cl_device_id device = create_device();
    cl_context context = clCreateContext(NULL, 1, &device, NULL, NULL, NULL);
    cl_queue_properties queue_props[] = {CL_QUEUE_PROPERTIES, CL_QUEUE_PROFILING_ENABLE, 0 // Terminator
    };
    cl_command_queue queue = clCreateCommandQueueWithProperties(context, device, queue_props, NULL);

    // Create buffers
    cl_mem im0_buf = clCreateBuffer(context, CL_MEM_READ_ONLY, l_width*l_height*4, NULL, NULL);
    cl_mem im1_buf = clCreateBuffer(context, CL_MEM_READ_ONLY, r_width*r_height*4, NULL, NULL);

    
    
    
    /*....................Transfer data from HOST to the DEVICE..................*/
    // Transfer the left and the right images to the DEVICE
    cl_event write_events[2];
    clEnqueueWriteBuffer(queue, im0_buf, CL_TRUE, 0, l_width*l_height*4, im0_data, 0, NULL, &write_events[0]);
    clEnqueueWriteBuffer(queue, im1_buf, CL_TRUE, 0, r_width*r_height*4, im1_data, 0, NULL, &write_events[1]);

    
    /*...........Image RESIZING using kernel and DEVICE..................*/
    // This variables are common for both image resize operation
    cl_program resize_prog = build_program(context, device, "resize.cl", NULL);
    cl_kernel resize_kernel = clCreateKernel(resize_prog, "resize", NULL);
    cl_mem resized_left_buf = clCreateBuffer(context, CL_MEM_WRITE_ONLY, WIDTH*HEIGHT*4, NULL, NULL);
    cl_mem resized_right_buf = clCreateBuffer(context, CL_MEM_WRITE_ONLY, WIDTH * HEIGHT * 4, NULL, NULL);
    cl_event resize_events[2];
    size_t global_size_resize[2] = {WIDTH, HEIGHT};

    //Resize left image (im0.png)
    clSetKernelArg(resize_kernel, 0, sizeof(cl_mem), &im0_buf);
    clSetKernelArg(resize_kernel, 1, sizeof(cl_mem), &resized_left_buf);
    clSetKernelArg(resize_kernel, 2, sizeof(int), &ORIG_WIDTH);
    clSetKernelArg(resize_kernel, 3, sizeof(int), &ORIG_HEIGHT);
    clEnqueueNDRangeKernel(queue, resize_kernel, 2, NULL, global_size_resize, NULL, 0, NULL, &resize_events[0]);


    //Resize right image (im1.png)
    clSetKernelArg(resize_kernel, 0, sizeof(cl_mem), &im1_buf);
    clSetKernelArg(resize_kernel, 1, sizeof(cl_mem), &resized_right_buf);
    clSetKernelArg(resize_kernel, 2, sizeof(int), &ORIG_WIDTH);
    clSetKernelArg(resize_kernel, 3, sizeof(int), &ORIG_HEIGHT);
    clEnqueueNDRangeKernel(queue, resize_kernel, 2, NULL, global_size_resize, NULL, 0, NULL, &resize_events[1]);


    // Read back resized image
    unsigned char *resized_left_img = (unsigned char*)malloc(WIDTH*HEIGHT*4);
    unsigned char *resized_right_img = (unsigned char*)malloc(WIDTH * HEIGHT * 4);
    cl_event read_resize_events[2];

    clEnqueueReadBuffer(queue, resized_left_buf, CL_TRUE, 0, WIDTH*HEIGHT*4, resized_left_img, 0, NULL, &read_resize_events[0]);
    clEnqueueReadBuffer(queue, resized_right_buf, CL_TRUE, 0, WIDTH*HEIGHT*4, resized_right_img, 0, NULL, &read_resize_events[1]);
    
    lodepng_encode32_file("output_opencl/resized_left.png", resized_left_img, WIDTH, HEIGHT);
    lodepng_encode32_file("output_opencl/resized_right.png", resized_right_img, WIDTH, HEIGHT);






    /*.............Convert RGBA image to Grayscale image............*/
    // Convert to grayscale
    cl_program gray_prog = build_program(context, device, "grayscale.cl", NULL);
    cl_kernel gray_kernel = clCreateKernel(gray_prog, "rgba_to_grayscale", NULL);
    cl_mem gray_left_buf = clCreateBuffer(context, CL_MEM_WRITE_ONLY, WIDTH*HEIGHT, NULL, NULL); // Single channel
    cl_mem gray_right_buf = clCreateBuffer(context, CL_MEM_WRITE_ONLY, WIDTH*HEIGHT, NULL, NULL); // Single channel
    size_t global_size_gray[2] = {WIDTH, HEIGHT};
    cl_event gray_events[2];

    clSetKernelArg(gray_kernel, 0, sizeof(cl_mem), &resized_left_buf);
    clSetKernelArg(gray_kernel, 1, sizeof(cl_mem), &gray_left_buf);
    clEnqueueNDRangeKernel(queue, gray_kernel, 2, NULL, global_size_gray, NULL, 1, &resize_events[0], &gray_events[0]);

    clSetKernelArg(gray_kernel, 0, sizeof(cl_mem), &resized_right_buf);
    clSetKernelArg(gray_kernel, 1, sizeof(cl_mem), &gray_right_buf);
    clEnqueueNDRangeKernel(queue, gray_kernel, 2, NULL, global_size_gray, NULL, 1, &resize_events[0], &gray_events[1]);

    // Read grayscale
    unsigned char *gray_left_img = (unsigned char*)malloc(WIDTH*HEIGHT);
    unsigned char *gray_right_img = (unsigned char*)malloc(WIDTH*HEIGHT);
    cl_event read_gray_events[2];

    clEnqueueReadBuffer(queue, gray_left_buf, CL_TRUE, 0, WIDTH*HEIGHT, gray_left_img, 0, NULL, &read_gray_events[0]);
    clEnqueueReadBuffer(queue, gray_right_buf, CL_TRUE, 0, WIDTH*HEIGHT, gray_right_img, 0, NULL, &read_gray_events[1]);
    lodepng_encode_file("output_opencl/gray_left.png", gray_left_img, WIDTH, HEIGHT, LCT_GREY, 8);
    lodepng_encode_file("output_opencl/gray_right.png", gray_right_img, WIDTH, HEIGHT, LCT_GREY, 8);
    
    /*..........End of RGBA to grayscale conversion..........*/





    /*...........................Apply moving average filter to the normalized depth map............*/
    // Apply 5x5 moving average
    size_t global_size_moving_average[2] = {WIDTH, HEIGHT};
    cl_program filter_prog = build_program(context, device, "moving_average.cl", NULL);
    cl_kernel filter_kernel = clCreateKernel(filter_prog, "moving_average_5x5", NULL);
    cl_mem filtered_occlusion_buff = clCreateBuffer(context, CL_MEM_WRITE_ONLY, WIDTH*HEIGHT, NULL, NULL);
    
    clSetKernelArg(filter_kernel, 0, sizeof(cl_mem), &gray_left_buf);
    clSetKernelArg(filter_kernel, 1, sizeof(cl_mem), &filtered_occlusion_buff);
    clSetKernelArg(filter_kernel, 2, sizeof(int), &WIDTH);
    clSetKernelArg(filter_kernel, 3, sizeof(int), &HEIGHT);
    
    cl_event filter_event;
    clEnqueueNDRangeKernel(queue, filter_kernel, 2, NULL, global_size_moving_average, NULL, 1, &gray_events[0], &filter_event);
    
    // Read final result
    unsigned char *filtered_img = (unsigned char*)malloc(WIDTH * HEIGHT);
    cl_event read_filter_event;
    
    clEnqueueReadBuffer(queue, filtered_occlusion_buff, CL_TRUE, 0, WIDTH*HEIGHT, filtered_img, 1, &filter_event, &read_filter_event);
    
    lodepng_encode_file("output_opencl/moving_average_filtered.png", filtered_img, WIDTH, HEIGHT, LCT_GREY, 8);
    /*...........................END of Applying moving average filter to the normalized depth map............*/




    /*...............Profiling data transfer and kernel execution time.................*/
    // Print profiling info
    print_profiling_info("Host to device transfer time for left image:", write_events[0]);
    print_profiling_info("Host to device transfer time for right image:", write_events[1]);
    print_profiling_info("Left image resize", resize_events[0]);
    print_profiling_info("Right image resize", resize_events[1]);
    print_profiling_info("Device to Host transfer time for left resized image", read_resize_events[0]);
    print_profiling_info("Device to Host transfer time for right resized image", read_resize_events[1]);
    print_profiling_info("Left image grayscale conversion", gray_events[0]);
    print_profiling_info("Right image grayscale conversion", gray_events[1]);
    print_profiling_info("Device to Host transfer time for left grascale image", read_gray_events[0]);
    print_profiling_info("Device to Host transfer time for right grayscale image", read_gray_events[1]);
    
    print_profiling_info("Moving average filter kernel execution time", filter_event);
    print_profiling_info("Device to Host transfer time for filtered image", read_filter_event);




    
    
    /*................Cleanup............*/
    clReleaseMemObject(im0_buf);
    clReleaseMemObject(im1_buf);


    clReleaseMemObject(resized_left_buf);
    clReleaseMemObject(resized_right_buf);
    clReleaseMemObject(gray_left_buf);
    clReleaseMemObject(gray_right_buf);
    clReleaseMemObject(filtered_occlusion_buff);


    clReleaseKernel(resize_kernel);
    clReleaseKernel(gray_kernel);
    clReleaseKernel(filter_kernel);


    clReleaseProgram(resize_prog);
    clReleaseProgram(gray_prog);
    clReleaseProgram(filter_prog);


    free(im0_data);
    free(im1_data);

    free(resized_left_img);
    free(resized_right_img);

    free(gray_left_img);
    free(gray_right_img);

    free(filtered_img);

    return 0;
}