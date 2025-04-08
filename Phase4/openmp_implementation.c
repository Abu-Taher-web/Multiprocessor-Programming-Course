#include <stdio.h>
#include <omp.h>

int main() {
    // This directive creates a parallel region
    #pragma omp parallel
    {
        // Each thread executes this block concurrently.
        int thread_id = omp_get_thread_num();
        printf("Hello from thread %d\n", thread_id);
    }
    return 0;
}
