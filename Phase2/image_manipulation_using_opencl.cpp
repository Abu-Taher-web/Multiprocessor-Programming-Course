#include "lodepng.h"
#include <CL/cl.h>
#include <iostream>
#include <vector>
#include <chrono>

// Function to read an image using LodePNG
std::vector<unsigned char> ReadImage(const char* filename, unsigned& width, unsigned& height) {
    std::vector<unsigned char> image;
    unsigned error = lodepng::decode(image, width, height, filename);

    if (error) {
        std::cerr << "Error reading image: " << lodepng_error_text(error) << std::endl;
        exit(1);
    }

    return image;
}

// Function to write an image using LodePNG
void WriteImage(const char* filename, const std::vector<unsigned char>& image, unsigned width, unsigned height) {
    unsigned error = lodepng::encode(filename, image, width, height, LCT_GREY);

    if (error) {
        std::cerr << "Error writing image: " << lodepng_error_text(error) << std::endl;
        exit(1);
    }
}

// OpenCL kernel source code
const char* kernel_source = R"(
    __kernel void ResizeImage(__global const uchar4* input, __global uchar4* output, int width, int height) {
        int x = get_global_id(0);
        int y = get_global_id(1);
        int new_width = width / 4;
        int new_height = height / 4;

        if (x < new_width && y < new_height) {
            int orig_x = x * 4;
            int orig_y = y * 4;
            int orig_index = orig_y * width + orig_x;
            int new_index = y * new_width + x;

            output[new_index] = input[orig_index];
        }
    }

    __kernel void GrayScaleImage(__global const uchar4* input, __global uchar* output, int width, int height) {
        int x = get_global_id(0);
        int y = get_global_id(1);

        if (x < width && y < height) {
            int index = y * width + x;
            uchar4 pixel = input[index];
            output[index] = (uchar)(0.2126f * pixel.x + 0.7152f * pixel.y + 0.0722f * pixel.z);
        }
    }

    __kernel void ApplyFilter(__global const uchar* input, __global uchar* output, int width, int height) {
        int x = get_global_id(0);
        int y = get_global_id(1);

        if (x < width && y < height) {
            float sum = 0.0f;

            // Flattened 1D Gaussian kernel
            const int gaussian_kernel[25] = {
                1, 4, 6, 4, 1,
                4, 16, 24, 16, 4,
                6, 24, 36, 24, 6,
                4, 16, 24, 16, 4,
                1, 4, 6, 4, 1
            };
            const int kernel_sum = 256;

            for (int ky = -2; ky <= 2; ky++) {
                for (int kx = -2; kx <= 2; kx++) {
                    // Clamp coordinates to image boundaries
                    int pixel_x = clamp(x + kx, 0, width - 1);
                    int pixel_y = clamp(y + ky, 0, height - 1);
                    int pixel_value = input[pixel_y * width + pixel_x];
                    int kernel_index = (ky + 2) * 5 + (kx + 2);
                    sum += pixel_value * gaussian_kernel[kernel_index];
                }
            }

            output[y * width + x] = (uchar)(sum / kernel_sum);
        }
    }
)";

// OpenCL initialization
cl_device_id device_id;
cl_context context;
cl_command_queue queue;
cl_program program;

void CheckError(cl_int err, const char* operation) {
    if (err != CL_SUCCESS) {
        std::cerr << "Error during " << operation << ": " << err << std::endl;
        exit(1);
    }
}

void InitializeOpenCL() {
    cl_platform_id platform_id;
    cl_uint ret_num_devices, ret_num_platforms;
    cl_int ret = clGetPlatformIDs(1, &platform_id, &ret_num_platforms);
    CheckError(ret, "clGetPlatformIDs");

    ret = clGetDeviceIDs(platform_id, CL_DEVICE_TYPE_GPU, 1, &device_id, &ret_num_devices);
    CheckError(ret, "clGetDeviceIDs");

    context = clCreateContext(NULL, 1, &device_id, NULL, NULL, &ret);
    CheckError(ret, "clCreateContext");

    // Use clCreateCommandQueueWithProperties instead of clCreateCommandQueue
    cl_command_queue_properties properties = CL_QUEUE_PROFILING_ENABLE;
    queue = clCreateCommandQueueWithProperties(context, device_id, &properties, &ret);
    CheckError(ret, "clCreateCommandQueueWithProperties");

    program = clCreateProgramWithSource(context, 1, &kernel_source, NULL, &ret);
    CheckError(ret, "clCreateProgramWithSource");

    ret = clBuildProgram(program, 1, &device_id, NULL, NULL, NULL);
    if (ret != CL_SUCCESS) {
        char build_log[4096];
        clGetProgramBuildInfo(program, device_id, CL_PROGRAM_BUILD_LOG, sizeof(build_log), build_log, NULL);
        std::cerr << "OpenCL build error:\n" << build_log << std::endl;
        exit(1);
    }
}

// Function to execute an OpenCL kernel
void ExecuteKernel(cl_kernel kernel, cl_mem input, cl_mem output, int width, int height, const char* kernel_name) {
    cl_int ret;
    cl_event event;

    size_t global_work_size[2] = {static_cast<size_t>(width), static_cast<size_t>(height)};
    ret = clEnqueueNDRangeKernel(queue, kernel, 2, NULL, global_work_size, NULL, 0, NULL, &event);
    CheckError(ret, "clEnqueueNDRangeKernel");

    clWaitForEvents(1, &event);

    cl_ulong start, end;
    clGetEventProfilingInfo(event, CL_PROFILING_COMMAND_START, sizeof(start), &start, NULL);
    clGetEventProfilingInfo(event, CL_PROFILING_COMMAND_END, sizeof(end), &end, NULL);

    double time = (end - start) * 1e-9; // Convert to seconds
    std::cout << kernel_name << " execution time: " << time << " seconds" << std::endl;
}

int main() {
    const char* input_filename = "im0.png";
    const char* output_filename = "image_0_bw_opencl.png";
    unsigned width, height;

    // Read the image
    std::vector<unsigned char> image = ReadImage(input_filename, width, height);

    // Initialize OpenCL
    InitializeOpenCL();

    // Create OpenCL buffers
    cl_int ret;
    cl_mem input_buffer = clCreateBuffer(context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, width * height * 4, image.data(), &ret);
    CheckError(ret, "clCreateBuffer (input)");

    int resized_width = width / 4;
    int resized_height = height / 4;
    cl_mem resized_buffer = clCreateBuffer(context, CL_MEM_READ_WRITE, resized_width * resized_height * 4, NULL, &ret);
    CheckError(ret, "clCreateBuffer (resized)");

    cl_mem grayscale_buffer = clCreateBuffer(context, CL_MEM_READ_WRITE, resized_width * resized_height, NULL, &ret);
    CheckError(ret, "clCreateBuffer (grayscale)");

    cl_mem filtered_buffer = clCreateBuffer(context, CL_MEM_READ_WRITE, resized_width * resized_height, NULL, &ret);
    CheckError(ret, "clCreateBuffer (filtered)");

    // Create kernels
    cl_kernel resize_kernel = clCreateKernel(program, "ResizeImage", &ret);
    CheckError(ret, "clCreateKernel (ResizeImage)");

    cl_kernel grayscale_kernel = clCreateKernel(program, "GrayScaleImage", &ret);
    CheckError(ret, "clCreateKernel (GrayScaleImage)");

    cl_kernel filter_kernel = clCreateKernel(program, "ApplyFilter", &ret);
    CheckError(ret, "clCreateKernel (ApplyFilter)");

    // Resize the image
    ret = clSetKernelArg(resize_kernel, 0, sizeof(cl_mem), &input_buffer);
    CheckError(ret, "clSetKernelArg (ResizeImage, input)");

    ret = clSetKernelArg(resize_kernel, 1, sizeof(cl_mem), &resized_buffer);
    CheckError(ret, "clSetKernelArg (ResizeImage, output)");

    ret = clSetKernelArg(resize_kernel, 2, sizeof(int), &resized_width);
    CheckError(ret, "clSetKernelArg (ResizeImage, width)");

    ret = clSetKernelArg(resize_kernel, 3, sizeof(int), &resized_height);
    CheckError(ret, "clSetKernelArg (ResizeImage, height)");

    ExecuteKernel(resize_kernel, input_buffer, resized_buffer, resized_width, resized_height, "ResizeImage");

    // Convert to grayscale
    ret = clSetKernelArg(grayscale_kernel, 0, sizeof(cl_mem), &resized_buffer);
    CheckError(ret, "clSetKernelArg (GrayScaleImage, input)");

    ret = clSetKernelArg(grayscale_kernel, 1, sizeof(cl_mem), &grayscale_buffer);
    CheckError(ret, "clSetKernelArg (GrayScaleImage, output)");

    ret = clSetKernelArg(grayscale_kernel, 2, sizeof(int), &resized_width);
    CheckError(ret, "clSetKernelArg (GrayScaleImage, width)");

    ret = clSetKernelArg(grayscale_kernel, 3, sizeof(int), &resized_height);
    CheckError(ret, "clSetKernelArg (GrayScaleImage, height)");

    ExecuteKernel(grayscale_kernel, resized_buffer, grayscale_buffer, resized_width, resized_height, "GrayScaleImage");

    // Apply 5x5 Gaussian blur
    ret = clSetKernelArg(filter_kernel, 0, sizeof(cl_mem), &grayscale_buffer);
    CheckError(ret, "clSetKernelArg (ApplyFilter, input)");

    ret = clSetKernelArg(filter_kernel, 1, sizeof(cl_mem), &filtered_buffer);
    CheckError(ret, "clSetKernelArg (ApplyFilter, output)");

    ret = clSetKernelArg(filter_kernel, 2, sizeof(int), &resized_width);
    CheckError(ret, "clSetKernelArg (ApplyFilter, width)");

    ret = clSetKernelArg(filter_kernel, 3, sizeof(int), &resized_height);
    CheckError(ret, "clSetKernelArg (ApplyFilter, height)");

    ExecuteKernel(filter_kernel, grayscale_buffer, filtered_buffer, resized_width, resized_height, "ApplyFilter");

    // Read the result back to host
    std::vector<unsigned char> output_image(resized_width * resized_height);
    ret = clEnqueueReadBuffer(queue, filtered_buffer, CL_TRUE, 0, resized_width * resized_height, output_image.data(), 0, NULL, NULL);
    CheckError(ret, "clEnqueueReadBuffer");

    // Write the resulting image
    WriteImage(output_filename, output_image, resized_width, resized_height);

    // Cleanup
    clReleaseMemObject(input_buffer);
    clReleaseMemObject(resized_buffer);
    clReleaseMemObject(grayscale_buffer);
    clReleaseMemObject(filtered_buffer);
    clReleaseKernel(resize_kernel);
    clReleaseKernel(grayscale_kernel);
    clReleaseKernel(filter_kernel);
    clReleaseProgram(program);
    clReleaseCommandQueue(queue);
    clReleaseContext(context);

    return 0;
}
//./image_manipulation_using_opencl.exe