#include <stdio.h>
#include <stdlib.h>
#include <CL/cl.h>
#include "lodepng.h"

const unsigned MAX_DISP = 65;
const unsigned WINDOW_SIZE = 4;
const unsigned THRESHOLD = 8;
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

void normalize_occlusion(unsigned char* occlusion_img, int width, int height) {
    unsigned char max_val = 0;
    unsigned char min_val = UCHAR_MAX;

    // First pass: find actual min/max values in the image
    for (int i = 0; i < width * height; i++) {
        if (occlusion_img[i] > max_val) max_val = occlusion_img[i];
        if (occlusion_img[i] < min_val) min_val = occlusion_img[i];
    }

    // Second pass: apply normalization
    unsigned char range = max_val - min_val;
    //printf("max: %s, Min: %s", max_val, min_val);
    for (int i = 0; i < width * height; i++) {
        if (range != 0) {
            occlusion_img[i] = (unsigned char)(255 * (occlusion_img[i] - min_val) / range);
        } else {
            occlusion_img[i] = 0;
        }
    }
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
    
    lodepng_encode32_file("resized_left.png", resized_left_img, WIDTH, HEIGHT);
    lodepng_encode32_file("resized_right.png", resized_right_img, WIDTH, HEIGHT);






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
    lodepng_encode_file("gray_left.png", gray_left_img, WIDTH, HEIGHT, LCT_GREY, 8);
    lodepng_encode_file("gray_right.png", gray_right_img, WIDTH, HEIGHT, LCT_GREY, 8);
    
    /*..........End of RGBA to grayscale conversion..........*/




    /*.................Disparity calculation using ZNCC.............*/

    cl_program zncc_prog_left = build_program(context, device, "zncc_left_register_usage_reduction.cl", "-cl-fast-relaxed-math -cl-mad-enable");
    cl_program zncc_prog_right = build_program(context, device, "zncc_right.cl", "-cl-fast-relaxed-math -cl-mad-enable");
    cl_kernel zncc_left_to_right_kernel = clCreateKernel(zncc_prog_left, "zncc_disparity_left", NULL);
    cl_kernel zncc_right_to_left_kernel = clCreateKernel(zncc_prog_right, "zncc_disparity_right", NULL);





    cl_mem disparity_left_buf = clCreateBuffer(context, CL_MEM_WRITE_ONLY, WIDTH*HEIGHT, NULL, NULL);
    cl_mem disparity_right_buf = clCreateBuffer(context, CL_MEM_WRITE_ONLY, WIDTH*HEIGHT, NULL, NULL);
    size_t global_size_zncc[2] = {WIDTH, HEIGHT};
    cl_event zncc_events[2];

    clSetKernelArg(zncc_left_to_right_kernel, 0, sizeof(cl_mem), &gray_left_buf);
    clSetKernelArg(zncc_left_to_right_kernel, 1, sizeof(cl_mem), &gray_right_buf);
    clSetKernelArg(zncc_left_to_right_kernel, 2, sizeof(cl_mem), &disparity_left_buf);
    clSetKernelArg(zncc_left_to_right_kernel, 3, sizeof(int), &WIDTH);
    clSetKernelArg(zncc_left_to_right_kernel, 4, sizeof(int), &HEIGHT);
    clSetKernelArg(zncc_left_to_right_kernel, 5, sizeof(int), &MAX_DISP);
    clSetKernelArg(zncc_left_to_right_kernel, 6, sizeof(int), &WINDOW_SIZE);
    clEnqueueNDRangeKernel(queue, zncc_left_to_right_kernel, 2, NULL, global_size_zncc, NULL, 0, NULL, &zncc_events[0]);

    clSetKernelArg(zncc_right_to_left_kernel, 0, sizeof(cl_mem), &gray_right_buf);
    clSetKernelArg(zncc_right_to_left_kernel, 1, sizeof(cl_mem), &gray_left_buf);
    clSetKernelArg(zncc_right_to_left_kernel, 2, sizeof(cl_mem), &disparity_right_buf);
    clSetKernelArg(zncc_right_to_left_kernel, 3, sizeof(int), &WIDTH);
    clSetKernelArg(zncc_right_to_left_kernel, 4, sizeof(int), &HEIGHT);
    clSetKernelArg(zncc_right_to_left_kernel, 5, sizeof(int), &MAX_DISP);
    clSetKernelArg(zncc_right_to_left_kernel, 6, sizeof(int), &WINDOW_SIZE);
    clEnqueueNDRangeKernel(queue, zncc_right_to_left_kernel, 2, NULL, global_size_zncc, NULL, 0, NULL, &zncc_events[1]);

    // Read disparity map
    unsigned char *disparity_left_img = (unsigned char*)malloc(WIDTH * HEIGHT);
    unsigned char *disparity_right_img = (unsigned char*)malloc(WIDTH * HEIGHT);
    cl_event read_disparity_events[2];
    clEnqueueReadBuffer(queue, disparity_left_buf, CL_TRUE, 0, WIDTH*HEIGHT, disparity_left_img, 0, NULL, &read_disparity_events[0]);
    clEnqueueReadBuffer(queue, disparity_right_buf, CL_TRUE, 0, WIDTH*HEIGHT, disparity_right_img, 0, NULL, &read_disparity_events[1]);
    lodepng_encode_file("disparity_left.png", disparity_left_img, WIDTH, HEIGHT, LCT_GREY, 8);
    lodepng_encode_file("disparity_right.png", disparity_right_img, WIDTH, HEIGHT, LCT_GREY, 8);

    /*.................End of disparity calculation using ZNCC.............*/



    /*....................Cross checking of left and right disparity image........*/
    cl_program cross_check_prog = build_program(context, device, "cross_check.cl", NULL);
    cl_kernel cross_check_kernel = clCreateKernel(cross_check_prog, "cross_check", NULL);
    cl_mem cross_checked_buff = clCreateBuffer(context, CL_MEM_WRITE_ONLY, WIDTH*HEIGHT, NULL, NULL);
    size_t global_size_cross[2] = {WIDTH, HEIGHT};
    cl_event cross_check_kernel_event;

    clSetKernelArg(cross_check_kernel, 0, sizeof(cl_mem), &disparity_left_buf);
    clSetKernelArg(cross_check_kernel, 1, sizeof(cl_mem), &disparity_right_buf);
    clSetKernelArg(cross_check_kernel, 2, sizeof(cl_mem), &cross_checked_buff);
    clSetKernelArg(cross_check_kernel, 3, sizeof(int), &WIDTH);
    clSetKernelArg(cross_check_kernel, 4, sizeof(int), &THRESHOLD);
    clEnqueueNDRangeKernel(queue, cross_check_kernel, 2, NULL, global_size_cross, NULL, 2, zncc_events, &cross_check_kernel_event);

// Read final disparity
unsigned char *cross_checked_img = (unsigned char*)malloc(WIDTH * HEIGHT);
cl_event read_cross_checked_buff_event;
clEnqueueReadBuffer(queue, cross_checked_buff, CL_TRUE, 0, WIDTH*HEIGHT, cross_checked_img, 1, &cross_check_kernel_event, &read_cross_checked_buff_event);
lodepng_encode_file("cross_checked.png", cross_checked_img, WIDTH, HEIGHT, LCT_GREY, 8);
    /*.....................End of Crosse checking.................................*/







    /*...........Occlusion Fill....................................................*/
    cl_program occlusion_prog = build_program(context, device, "occlusion.cl", NULL);
    cl_kernel occlusion_kernel = clCreateKernel(occlusion_prog, "occlusion_filling", NULL);
    cl_mem occlusion_buff = clCreateBuffer(context, CL_MEM_WRITE_ONLY, WIDTH*HEIGHT, NULL, NULL);
    size_t global_size_occlusion[2] = {WIDTH, HEIGHT};
    cl_event occlusion_kernel_event;

    clSetKernelArg(occlusion_kernel, 0, sizeof(cl_mem), &cross_checked_buff);
    clSetKernelArg(occlusion_kernel, 1, sizeof(cl_mem), &occlusion_buff);
    clSetKernelArg(occlusion_kernel, 2, sizeof(int), &WIDTH);
    clSetKernelArg(occlusion_kernel, 3, sizeof(int), &HEIGHT);
    clSetKernelArg(occlusion_kernel, 4, sizeof(int), &MAX_SEARCH_RADIUS);
    clEnqueueNDRangeKernel(queue, occlusion_kernel, 2, NULL, global_size_occlusion, NULL, 1, &cross_check_kernel_event, &occlusion_kernel_event);

    // Transfer occlusion filled image from DEVICE to the HOST
    unsigned char *occlusion_img = (unsigned char*)malloc(WIDTH * HEIGHT);
    cl_event read_occlusion_event;
    clEnqueueReadBuffer(queue, occlusion_buff, CL_TRUE, 0, WIDTH*HEIGHT, occlusion_img, 1, &occlusion_kernel_event, &read_occlusion_event);
    
    
    // Normalize occlusion image from [0-64] to [0-255]
    
    normalize_occlusion(occlusion_img, WIDTH, HEIGHT);
    /*
    for (int i = 0; i < WIDTH*HEIGHT; i++)
    {
        if (0 <= occlusion_img[i] && occlusion_img[i]<= 50)
        {
            occlusion_img[i] = 80;
        }
        
    }
    */

    lodepng_encode_file("occlusion_filled.png", occlusion_img, WIDTH, HEIGHT, LCT_GREY, 8);

    /*...........END OF Occlusion Fill....................................................*/


    /*...........................Apply moving average filter to the normalized depth map............*/
    // Apply 5x5 moving average
    cl_program filter_prog = build_program(context, device, "moving_average.cl", NULL);
    cl_kernel filter_kernel = clCreateKernel(filter_prog, "moving_average_5x5", NULL);
    cl_mem filtered_occlusion_buff = clCreateBuffer(context, CL_MEM_WRITE_ONLY, WIDTH*HEIGHT, NULL, NULL);

    clSetKernelArg(filter_kernel, 0, sizeof(cl_mem), &occlusion_buff);
    clSetKernelArg(filter_kernel, 1, sizeof(cl_mem), &filtered_occlusion_buff);
    clSetKernelArg(filter_kernel, 2, sizeof(int), &WIDTH);
    clSetKernelArg(filter_kernel, 3, sizeof(int), &HEIGHT);

    cl_event filter_event;
    clEnqueueNDRangeKernel(queue, filter_kernel, 2, NULL, global_size_occlusion, NULL, 1, &occlusion_kernel_event, &filter_event);

    // Read final result
    unsigned char *filtered_img = (unsigned char*)malloc(WIDTH * HEIGHT);
    cl_event read_filter_event;
    
    clEnqueueReadBuffer(queue, filtered_occlusion_buff, CL_TRUE, 0, WIDTH*HEIGHT, filtered_img, 1, &filter_event, &read_filter_event);
    for (int i = 0; i < WIDTH * HEIGHT; i++) {
        // Scale with rounding to nearest integer
        filtered_img[i] = (unsigned char)((filtered_img[i] * 255) / MAX_DISP);
    }

    lodepng_encode_file("filtered_occlusion.png", filtered_img, WIDTH, HEIGHT, LCT_GREY, 8);
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
    print_profiling_info("Left disparity calculation: Left -> Right", zncc_events[0]);
    print_profiling_info("Right disparity calculation: Right -> Left", zncc_events[1]);
    print_profiling_info("Device to Host transfer time for left disparity", read_disparity_events[0]);
    print_profiling_info("Device to Host transfer time for right disparity", read_disparity_events[1]);
    print_profiling_info("Cross checked kernel execution time", cross_check_kernel_event);
    print_profiling_info("Device to Host transfer time for cross checked image", read_cross_checked_buff_event);
    print_profiling_info("Occlusion kernel execution time", occlusion_kernel_event);
    print_profiling_info("Device to Host transfer time for occlusion filled image", read_occlusion_event);




    
    
    /*................Cleanup............*/
    clReleaseMemObject(im0_buf);
    clReleaseMemObject(im1_buf);


    clReleaseMemObject(resized_left_buf);
    clReleaseMemObject(resized_right_buf);
    clReleaseMemObject(gray_left_buf);
    clReleaseMemObject(gray_right_buf);
    clReleaseMemObject(disparity_left_buf);
    clReleaseMemObject(disparity_right_buf);
    clReleaseMemObject(cross_checked_buff);
    clReleaseMemObject(occlusion_buff);


    clReleaseKernel(resize_kernel);
    clReleaseKernel(gray_kernel);
    clReleaseKernel(zncc_left_to_right_kernel);
    clReleaseKernel(zncc_right_to_left_kernel);
    clReleaseKernel(cross_check_kernel);
    clReleaseKernel(occlusion_kernel);


    clReleaseProgram(resize_prog);
    clReleaseProgram(gray_prog);
    clReleaseProgram(zncc_prog_left);
    clReleaseProgram(zncc_prog_right);
    clReleaseProgram(cross_check_prog);
    clReleaseProgram(occlusion_prog);


    free(im0_data);
    free(im1_data);

    free(resized_left_img);
    free(resized_right_img);

    free(gray_left_img);
    free(gray_right_img);

    free(disparity_left_img);
    free(disparity_right_img);
    free(cross_checked_img);
    free(occlusion_img);

    return 0;
}