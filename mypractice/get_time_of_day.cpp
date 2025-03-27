#include <stdio.h>
#include <sys/time.h>

int main(void) {
    struct timeval start, end;
    long seconds, useconds;
    double elapsed;

    // Get the starting time
    gettimeofday(&start, NULL);

    // Example computation: sum numbers 1 to 1,000,000
    volatile long sum = 0;
    for (long i = 1; i <= 1000000; i++) {
        sum += i;
    }

    // Get the ending time
    gettimeofday(&end, NULL);

    // Calculate the elapsed time in seconds and microseconds
    seconds  = end.tv_sec  - start.tv_sec;
    useconds = end.tv_usec - start.tv_usec;
    elapsed = seconds + useconds / 1e6;

    printf("Elapsed time: %.6f seconds\n", elapsed);
    return 0;
}
