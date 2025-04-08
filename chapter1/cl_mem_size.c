#include <CL/cl.h>
#include <stdio.h>

int main() {
    printf("Size of cl_mem: %zu bytes\n", sizeof(cl_mem));
    return 0;
}