#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>

void* abc(void* arg){
    int num = *((int*)arg);
    printf("argument value: %d\n", num);
    return NULL;
}

int main(){
    pthread_t thread;
    int value = 20;
    if(pthread_create(&thread, NULL, abc, &value)){
        fprintf(stderr, "Error creating thread\n");
        return 1;
    }

    if(pthread_join(thread, NULL)){
        fprintf(stderr, "error joining thread\n");
        return 2;
    }

    printf("Main thread finished.\n");
    return 0;
}