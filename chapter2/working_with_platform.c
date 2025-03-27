#include <CL/cl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>

int main(){
    cl_platform_id *platforms;
    cl_uint num_platforms;
    
    // cl_int clGetPlatformIDs(cl_uint num_entries, cl_platform_id *platforms, cl_uint *num_platforms)
    cl_int x = clGetPlatformIDs(5, NULL, &num_platforms);
    platforms = (cl_platform_id*)malloc(sizeof(cl_platform_id)*num_platforms);
    clGetPlatformIDs(num_platforms, platforms, NULL);

    /*
    cl_int clGetPlatformInfo(cl_platform_id platform, 
                            cl_platform_info param_name, 
                            size_t param_value_size,
                            void *param_value, 
                            size_t *param_value_size_ret)
    */
    char pform_name[40];
    char pform_vendor[40];
    char pform_version[40];
    char pform_profile[40];
    char pform_extention[100];
    clGetPlatformInfo(platforms[1], CL_PLATFORM_NAME, sizeof(pform_name), &pform_name, NULL);
    clGetPlatformInfo(platforms[1], CL_PLATFORM_VENDOR, sizeof(pform_vendor), &pform_vendor, NULL);
    clGetPlatformInfo(platforms[1], CL_PLATFORM_VERSION, sizeof(pform_version), &pform_version, NULL);
    clGetPlatformInfo(platforms[1], CL_PLATFORM_PROFILE, sizeof(pform_profile), &pform_profile, NULL);
    clGetPlatformInfo(platforms[1], CL_PLATFORM_EXTENSIONS, sizeof(pform_extention), &pform_extention, NULL);

    /*
    cl_int clGetDeviceIDs(cl_platform_id platform, 
                        cl_device_type device_type, cl_uint num_entries, 
                        cl_device_id *devices, cl_uint *num_devices)
    */

    cl_device_id* devs;
    clGetDeviceIDs(platforms[0], CL_DEVICE_TYPE_GPU, 3, devs, NULL);

    cl_uint num_devices_all;
    cl_uint num_devices_default;
    cl_uint num_devices_cpu;
    cl_uint num_devices_gpu;
    cl_uint num_devices_acc;
    clGetDeviceIDs(platforms[1], CL_DEVICE_TYPE_ALL, 100, NULL, &num_devices_all);
    clGetDeviceIDs(platforms[1], CL_DEVICE_TYPE_DEFAULT, 100, NULL, &num_devices_default);
    clGetDeviceIDs(platforms[1], CL_DEVICE_TYPE_CPU, 100, NULL, &num_devices_cpu);
    clGetDeviceIDs(platforms[1], CL_DEVICE_TYPE_GPU, 100, NULL, &num_devices_gpu);
    clGetDeviceIDs(platforms[1], CL_DEVICE_TYPE_ACCELERATOR, 100, NULL, &num_devices_acc);


    /*
     cl_int clGetDeviceInfo(cl_device_id device,
                            cl_device_info param_name, size_t param_value_size,
                            void *param_value, size_t *param_value_size_ret)
    */
    char device_name[40];
    clGetDeviceInfo(devs[0], CL_DEVICE_NAME, sizeof(device_name), &device_name, NULL);


    printf("%p\n", platforms);
    printf("%p\n", platforms+1);
    printf("%zu\n", sizeof(cl_platform_id));
    printf("%d\n",x);
    printf("%d\n", num_platforms);
    printf("%p\n", &platforms);
    printf("%p\n", platforms[0]);
    printf("%p\n", platforms[1]);
    printf("pform_name: %s\n", pform_name);
    printf("pform_vendor: %s\n", pform_vendor);
    printf("pform_version: %s\n", pform_version);
    printf("pform_profile: %s\n", pform_profile);
    printf("pform_extention: %s\n", pform_extention);
    printf("all device:%d\n",num_devices_all);
    printf("num_devices_default:%d\n",num_devices_default);
    printf("num_devices_cpu:%d\n",num_devices_cpu);
    printf("num_devices_gpu:%d\n",num_devices_gpu);
    printf("num_devices_acc:%d\n",num_devices_acc);
    printf("device_name:%s\n",device_name);
}

// ./working_with_platform.exe