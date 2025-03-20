#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>

#define SIZE 100

void add_Matrix(float *matrix1, float *matrix2, float *result, int size) {
    for (int i = 0; i < size * size; i++) {
        result[i] = matrix1[i] + matrix2[i];
    }
}

double getCurrentTime() {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return tv.tv_sec + tv.tv_usec * 1e-6;
}

int main() {
    int size = SIZE * SIZE;
    float *matrix1 = (float *)malloc(size * sizeof(float));
    float *matrix2 = (float *)malloc(size * sizeof(float));
    float *result = (float *)malloc(size * sizeof(float));

    // Initialize matrices with some values
    for (int i = 0; i < size; i++) {
        matrix1[i] = (float)i;
        matrix2[i] = (float)(i * 2);
    }

    double startTime = getCurrentTime();
    add_Matrix(matrix1, matrix2, result, SIZE);
    double endTime = getCurrentTime();

    printf("Host Execution Time: %f seconds\n", endTime - startTime);

    free(matrix1);
    free(matrix2);
    free(result);

    return 0;
}