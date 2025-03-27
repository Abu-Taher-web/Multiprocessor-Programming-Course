#include <CL/cl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <stdio.h>

int main(){
    
    //Getting the platform information
    // cl_int clGetPlatformIDs(cl_uint num_entries, cl_platform_id *platforms, cl_uint *num_platforms)
    cl_platform_id *platforms;
    cl_uint num_platforms;
    cl_int x = clGetPlatformIDs(5, NULL, &num_platforms);
    platforms = (cl_platform_id*)malloc(sizeof(cl_platform_id)*num_platforms);
    clGetPlatformIDs(num_platforms, platforms, NULL);




    //Getting details about each platform
    /*
    cl_int clGetPlatformInfo(cl_platform_id platform, 
                            cl_platform_info param_name, 
                            size_t param_value_size,
                            void *param_value, 
                            size_t *param_value_size_ret)
    */

    //This is the first platform's information
    char pform_name0[40];
    char pform_vendor0[40];
    char pform_version0[40];
    char pform_profile0[40];
    char pform_extention0[100];
    clGetPlatformInfo(platforms[0], CL_PLATFORM_NAME, sizeof(pform_name0), &pform_name0, NULL);
    clGetPlatformInfo(platforms[0], CL_PLATFORM_VENDOR, sizeof(pform_vendor0), &pform_vendor0, NULL);
    clGetPlatformInfo(platforms[0], CL_PLATFORM_VERSION, sizeof(pform_version0), &pform_version0, NULL);
    clGetPlatformInfo(platforms[0], CL_PLATFORM_PROFILE, sizeof(pform_profile0), &pform_profile0, NULL);
    clGetPlatformInfo(platforms[0], CL_PLATFORM_EXTENSIONS, sizeof(pform_extention0), &pform_extention0, NULL);
    printf("Platform 0:\n");
    printf("pform_name: %s\n", pform_name0);
    printf("pform_vendor: %s\n", pform_vendor0);
    printf("pform_version: %s\n", pform_version0);
    printf("pform_profile: %s\n", pform_profile0);
    printf("pform_extention: %s\n", pform_extention0);

    //This is the second platform's information.
    char pform_name1[40];
    char pform_vendor1[40];
    char pform_version1[40];
    char pform_profile1[40];
    char pform_extention1[100];
    clGetPlatformInfo(platforms[1], CL_PLATFORM_NAME, sizeof(pform_name1), &pform_name1, NULL);
    clGetPlatformInfo(platforms[1], CL_PLATFORM_VENDOR, sizeof(pform_vendor1), &pform_vendor1, NULL);
    clGetPlatformInfo(platforms[1], CL_PLATFORM_VERSION, sizeof(pform_version1), &pform_version1, NULL);
    clGetPlatformInfo(platforms[1], CL_PLATFORM_PROFILE, sizeof(pform_profile1), &pform_profile1, NULL);
    clGetPlatformInfo(platforms[1], CL_PLATFORM_EXTENSIONS, sizeof(pform_extention1), &pform_extention1, NULL);
    printf("\nPlatform 1:\n");
    printf("pform_name: %s\n", pform_name1);
    printf("pform_vendor: %s\n", pform_vendor1);
    printf("pform_version: %s\n", pform_version1);
    printf("pform_profile: %s\n", pform_profile1);
    printf("pform_extention: %s\n", pform_extention1);


    //Device info in each platform
    /*
    cl_int clGetDeviceIDs(cl_platform_id platform, 
                        cl_device_type device_type, cl_uint num_entries, 
                        cl_device_id *devices, cl_uint *num_devices)
    */

    
    //Number of cpu, gpu and other device available in the first platform
    cl_uint num_devices_all;
    cl_uint num_devices_default;
    cl_uint num_devices_cpu;
    cl_uint num_devices_gpu;
    cl_uint num_devices_acc;
    clGetDeviceIDs(platforms[0], CL_DEVICE_TYPE_ALL, 100, NULL, &num_devices_all);
    clGetDeviceIDs(platforms[0], CL_DEVICE_TYPE_DEFAULT, 100, NULL, &num_devices_default);
    clGetDeviceIDs(platforms[0], CL_DEVICE_TYPE_CPU, 100, NULL, &num_devices_cpu);
    clGetDeviceIDs(platforms[0], CL_DEVICE_TYPE_GPU, 100, NULL, &num_devices_gpu);
    clGetDeviceIDs(platforms[0], CL_DEVICE_TYPE_ACCELERATOR, 100, NULL, &num_devices_acc);
    
    printf("\nPlatform 0's device information:\n");
    printf("all device:%d\n",num_devices_all);
    printf("num_devices_default:%d\n",num_devices_default);
    printf("num_devices_cpu:%d\n",num_devices_cpu);
    printf("num_devices_gpu:%d\n",num_devices_gpu);
    printf("num_devices_acc:%d\n",num_devices_acc);

    //Number of cpu, gpu and other device available in the Second platform
    cl_uint num_devices_all2;
    cl_uint num_devices_default2;
    cl_uint num_devices_cpu2;
    cl_uint num_devices_gpu2;
    cl_uint num_devices_acc2;
    clGetDeviceIDs(platforms[1], CL_DEVICE_TYPE_ALL, 100, NULL, &num_devices_all2);
    clGetDeviceIDs(platforms[1], CL_DEVICE_TYPE_DEFAULT, 100, NULL, &num_devices_default2);
    clGetDeviceIDs(platforms[1], CL_DEVICE_TYPE_CPU, 100, NULL, &num_devices_cpu2);
    clGetDeviceIDs(platforms[1], CL_DEVICE_TYPE_GPU, 100, NULL, &num_devices_gpu2);
    clGetDeviceIDs(platforms[1], CL_DEVICE_TYPE_ACCELERATOR, 100, NULL, &num_devices_acc2);
    
    printf("\nPlatform 1's device information:\n");
    printf("all device:%d\n",num_devices_all2);
    printf("num_devices_default:%d\n",num_devices_default2);
    printf("num_devices_cpu:%d\n",num_devices_cpu2);
    printf("num_devices_gpu:%d\n",num_devices_gpu2);
    printf("num_devices_acc:%d\n",num_devices_acc2);

    /*
     cl_int clGetDeviceInfo(cl_device_id device,
                            cl_device_info param_name, size_t param_value_size,
                            void *param_value, size_t *param_value_size_ret)
    */
    cl_device_id* devs = (cl_device_id*)malloc(3 * sizeof(cl_device_id));
    clGetDeviceIDs(platforms[0], CL_DEVICE_TYPE_GPU, 3, devs, NULL);

    char device_name0[40];
    char device_vendor_name0[40];
    char device_extension0[40];
    cl_ulong mem_size0;
    cl_uint address_bits0;
    cl_bool available0;
    cl_bool compiler0;
    clGetDeviceInfo(devs[0], CL_DEVICE_NAME, sizeof(device_name0), &device_name0, NULL);
    clGetDeviceInfo(devs[0], CL_PLATFORM_VENDOR, sizeof(device_vendor_name0), &device_vendor_name0, NULL);
    clGetDeviceInfo(devs[0], CL_DEVICE_EXTENSIONS, sizeof(device_extension0), &device_extension0, NULL);
    clGetDeviceInfo(devs[0], CL_DEVICE_GLOBAL_MEM_SIZE, sizeof(mem_size0), &mem_size0, NULL);
    clGetDeviceInfo(devs[0], CL_DEVICE_ADDRESS_BITS, sizeof(address_bits0), &address_bits0, NULL);
    clGetDeviceInfo(devs[0], CL_DEVICE_AVAILABLE, sizeof(available0), &available0, NULL);
    clGetDeviceInfo(devs[0], CL_DEVICE_COMPILER_AVAILABLE, sizeof(compiler0), &compiler0, NULL);

    printf("\nPlatform 0 - Device - 0: (GPU) information\n");
    printf("device_name0:%s\n",device_name0);
    printf("device_vendor_name0:%s\n",device_vendor_name0);
    printf("device_extension:%s\n",device_extension0);
    printf("mem_size0:%d\n",mem_size0/ (1024 * 1024 * 1024));
    printf("address_bits0:%d\n",address_bits0);
    printf("available0:%d\n",available0);
    printf("compiler0:%d\n",compiler0);
    free(devs);
    
    // Device info for Intel GPU in Platform 1
cl_device_id* devs1 = (cl_device_id*)malloc(3 * sizeof(cl_device_id));
clGetDeviceIDs(platforms[1], CL_DEVICE_TYPE_GPU, 3, devs1, NULL);

char device_name1[40];
char device_vendor_name1[40];
char device_extension1[100];  // Increased size for Intel extensions
cl_ulong mem_size1;
cl_uint address_bits1;
cl_bool available1;
cl_bool compiler1;

clGetDeviceInfo(devs1[0], CL_DEVICE_NAME, sizeof(device_name1), &device_name1, NULL);
clGetDeviceInfo(devs1[0], CL_DEVICE_VENDOR, sizeof(device_vendor_name1), &device_vendor_name1, NULL);
clGetDeviceInfo(devs1[0], CL_DEVICE_EXTENSIONS, sizeof(device_extension1), &device_extension1, NULL);
clGetDeviceInfo(devs1[0], CL_DEVICE_GLOBAL_MEM_SIZE, sizeof(mem_size1), &mem_size1, NULL);
clGetDeviceInfo(devs1[0], CL_DEVICE_ADDRESS_BITS, sizeof(address_bits1), &address_bits1, NULL);
clGetDeviceInfo(devs1[0], CL_DEVICE_AVAILABLE, sizeof(available1), &available1, NULL);
clGetDeviceInfo(devs1[0], CL_DEVICE_COMPILER_AVAILABLE, sizeof(compiler1), &compiler1, NULL);

printf("\nPlatform 1 - Device - 0: (GPU) information\n");
printf("device_name1:%s\n", device_name1);
printf("device_vendor_name1:%s\n", device_vendor_name1);
printf("device_extension1:%s\n", device_extension1);
printf("mem_size1:%llu\n", mem_size1/ (1024 * 1024 * 1024));
printf("address_bits1:%u\n", address_bits1);
printf("available1:%d\n", available1);
printf("compiler1:%d\n", compiler1);

free(devs1);  // Free allocated memory

// ========== CONTEXT CREATION FOR PLATFORM 0's GPU ==========
    
    cl_device_id platform0_gpu;
    clGetDeviceIDs(platforms[0], CL_DEVICE_TYPE_GPU, 1, &platform0_gpu, NULL);
    cl_context_properties context_props[] = {CL_CONTEXT_PLATFORM, (cl_context_properties)platforms[0], 0};
    
    // Create the context
    cl_int err;
    cl_context context = clCreateContext(context_props, 1, &platform0_gpu, NULL, NULL, &err);

    
        cl_uint context_num_devices;
        cl_device_id context_devices[1];
        size_t props_size;
        cl_uint ref_count;
        cl_bool prefer_shared = CL_FALSE;
        
        clGetContextInfo(context, CL_CONTEXT_NUM_DEVICES, sizeof(cl_uint), &context_num_devices, NULL);
        clGetContextInfo(context, CL_CONTEXT_DEVICES, sizeof(context_devices), context_devices, NULL);
        clGetContextInfo(context, CL_CONTEXT_PROPERTIES, sizeof(context_props), context_props, &props_size);
        clGetContextInfo(context, CL_CONTEXT_REFERENCE_COUNT, sizeof(cl_uint), &ref_count, NULL);
        
        char device_name[100];
        clGetDeviceInfo(context_devices[0], CL_DEVICE_NAME, sizeof(device_name), device_name, NULL);
        
        printf("\nContext Information:\n");
        printf("-------------------\n");
        printf("Number of devices in context: %u\n", context_num_devices);
        printf("Device in context: %s\n", device_name);
        printf("Context properties:\n");
        printf("  CL_CONTEXT_PLATFORM: %p\n", (void*)context_props[1]);
        printf("  Terminator: 0\n");
        printf("Context reference count: %u\n", ref_count);

    // Create program
    FILE* program_handle = fopen("kernel.cl", "rb");
    printf("Address of the file at first:%p\n",program_handle);

    fseek(program_handle, 0, SEEK_END);
    printf("Address of the file at the end:%p\n",program_handle);

    size_t program_size = ftell(program_handle);
    printf("Programm size: %d\n", program_size);
    rewind(program_handle);
    printf("Address of the file after rewind:%p\n",program_handle);

    char* program_buffer = (char*)malloc(program_size);
    //program_buffer[program_size] = '\0';
    size_t bytes_read = fread(program_buffer, 1, program_size, program_handle);
    printf("Byte read:%d\n", bytes_read);
    fclose(program_handle);
    int i = 0;
    while (i < program_size)
    {
        printf("%c",program_buffer[i]);
        i++;
    }
    printf("\n last character: %c\n", program_buffer[program_size]);

    cl_int programm_error;
    //program_buffer[program_size-2] = '\0';
    cl_program program = clCreateProgramWithSource(context, 1, (const char**)&program_buffer, &program_size, &programm_error);
    
    printf("Program error: %d\n", programm_error);


    
    cl_int builderror = clBuildProgram(program, 1, (const cl_device_id*)&platform0_gpu, "-cl-fast-relaxed-math", NULL, NULL);
    if (builderror != CL_SUCCESS) {
        char build_log[4096];
        clGetProgramBuildInfo(program, platform0_gpu, CL_PROGRAM_BUILD_LOG, sizeof(build_log), build_log, NULL);
        printf("Build error:\n%s\n", build_log);
    }
    printf("Build error: %d", builderror);

    // First, get the source size
size_t source_size;
cl_int program_info_err = clGetProgramInfo(program, CL_PROGRAM_SOURCE, 0, NULL, &source_size);

// Allocate buffer and fetch the source
char *source = (char*)malloc(source_size);
program_info_err = clGetProgramInfo(program, CL_PROGRAM_SOURCE, source_size, source, NULL);

printf("\nKernel source:\n%s\n", source);
free(source);

    
    free(program_buffer);

    }
    

