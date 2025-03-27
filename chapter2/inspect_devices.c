#include <CL/cl.h>
#include <stdio.h>
#include <stdlib.h>

int main() {
    cl_platform_id platforms[2];
    cl_uint num_platforms;
    cl_int status;

    // Step 1: Get all platforms
    status = clGetPlatformIDs(2, platforms, &num_platforms);
    if (status != CL_SUCCESS) {
        printf("Error: Failed to get platform IDs! Error code: %d\n", status);
        return -1;
    }

    // Step 2: Iterate over platforms
    for (cl_uint i = 0; i < num_platforms; i++) {
        char platform_name[128];
        status = clGetPlatformInfo(platforms[i], CL_PLATFORM_NAME, sizeof(platform_name), platform_name, NULL);
        if (status != CL_SUCCESS) {
            printf("Error: Failed to get platform %d name! Error code: %d\n", i, status);
            continue;
        }
        printf("Platform %d: %s\n", i, platform_name);

        // Step 3: Get devices for the platform
        cl_device_id devices[10];
        cl_uint num_devices;
        status = clGetDeviceIDs(platforms[i], CL_DEVICE_TYPE_ALL, 10, devices, &num_devices);
        if (status != CL_SUCCESS) {
            printf("Error: Failed to get device IDs for platform %d! Error code: %d\n", i, status);
            continue;
        }
        printf("Number of devices: %u\n", num_devices);

        // Step 4: Inspect each device
        for (cl_uint j = 0; j < num_devices; j++) {
            char device_name[128];
            cl_device_type device_type;

            // Get device name
            status = clGetDeviceInfo(devices[j], CL_DEVICE_NAME, sizeof(device_name), device_name, NULL);
            if (status != CL_SUCCESS) {
                printf("Error: Failed to get device %d name! Error code: %d\n", j, status);
                continue;
            }

            // Get device type
            status = clGetDeviceInfo(devices[j], CL_DEVICE_TYPE, sizeof(device_type), &device_type, NULL);
            if (status != CL_SUCCESS) {
                printf("Error: Failed to get device %d type! Error code: %d\n", j, status);
                continue;
            }

            // Print device details
            printf("Device %d:\n", j);
            printf("  Name: %s\n", device_name);
            printf("  Type: ");
            if (device_type & CL_DEVICE_TYPE_CPU) printf("CPU ");
            if (device_type & CL_DEVICE_TYPE_GPU) printf("GPU ");
            if (device_type & CL_DEVICE_TYPE_ACCELERATOR) printf("Accelerator ");
            if (device_type & CL_DEVICE_TYPE_DEFAULT) printf("Default ");
            printf("\n");
        }

        printf("\n");
    }

    return 0;
}