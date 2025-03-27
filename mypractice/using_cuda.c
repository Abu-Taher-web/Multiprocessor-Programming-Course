#include <iostream>
#include <cuda_runtime.h>

__global__ void my_kernel(int *arr) {
    int idx = threadIdx.x;
    arr[idx] = idx * 2;
}

int main() {
    int n = 10;
    int arr[n];

    int *d_arr;
    cudaMalloc(&d_arr, n * sizeof(int));

    my_kernel<<<1, n>>>(d_arr);
    cudaMemcpy(arr, d_arr, n * sizeof(int), cudaMemcpyDeviceToHost);

    for (int i = 0; i < n; i++) {
        std::cout << "arr[" << i << "] = " << arr[i] << std::endl;
    }

    cudaFree(d_arr);
    return 0;
}
