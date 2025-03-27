#include <stdio.h>
#include <omp.h>

int main() {
    int n = 10;
    int arr[n];
    
    // Parallelize the for loop with OpenMP
    #pragma omp parallel for
    for (int i = 0; i < n; i++) {
        arr[i] = i * 2;
        printf("arr[%d] = %d\n", i, arr[i]);
    }
    
    return 0;
}
