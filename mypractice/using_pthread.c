#include <pthread.h>
#include <stdio.h>

// Function to be executed by each thread
void *my_function(void *arg) {
    int *num = (int *)arg;
    printf("Hello from thread! Argument value: %d\n", *num);
    return NULL;
}

int main() {
    pthread_t thread1, thread2;
    int arg1 = 10, arg2 = 20;

    // Create threads
    pthread_create(&thread1, NULL, my_function, (void *)&arg1);
    pthread_create(&thread2, NULL, my_function, (void *)&arg2);

    // Wait for threads to finish
    pthread_join(thread1, NULL);
    pthread_join(thread2, NULL);

    return 0;
}
