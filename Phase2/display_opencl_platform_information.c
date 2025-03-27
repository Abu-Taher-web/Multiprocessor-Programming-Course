#include <CL/cl.h>
#include <stdio.h>

void displayPlatformInfo() {
    cl_platform_id platform;
    clGetPlatformIDs(1, &platform, NULL);

    char platformName[128];
    char platformVendor[128];
    char platformVersion[128];

    clGetPlatformInfo(platform, CL_PLATFORM_NAME, sizeof(platformName), platformName, NULL);
    clGetPlatformInfo(platform, CL_PLATFORM_VENDOR, sizeof(platformVendor), platformVendor, NULL);
    clGetPlatformInfo(platform, CL_PLATFORM_VERSION, sizeof(platformVersion), platformVersion, NULL);

    printf("Platform Name: %s\n", platformName);
    printf("Platform Vendor: %s\n", platformVendor);
    printf("Platform Version: %s\n", platformVersion);
}

int main (){
    displayPlatformInfo();
}
//./display_opencl_platform_information.exe