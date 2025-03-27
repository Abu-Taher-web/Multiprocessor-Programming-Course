#include <windows.h>
#include <stdio.h>

int main(void) {
    LARGE_INTEGER frequency;
    LARGE_INTEGER start, end;
    double elapsed;

    // Retrieve the frequency of the high-resolution performance counter
    if (!QueryPerformanceFrequency(&frequency)) {
        fprintf(stderr, "High-resolution performance counter not supported.\n");
        return 1;
    }

    // Get the starting count
    QueryPerformanceCounter(&start);

    // Example computation: a simple loop
    volatile long sum = 0;
    for (long i = 0; i < 1000000; i++) {
        sum += i;
    }

    // Get the ending count
    QueryPerformanceCounter(&end);

    // Calculate elapsed time in seconds
    elapsed = (double)(end.QuadPart - start.QuadPart) / frequency.QuadPart;
    printf("Elapsed time: %.6f seconds\n", elapsed);

    return 0;
}
