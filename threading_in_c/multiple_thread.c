#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>

#define NUM_THREAD 3

void* abc(void* arg){
    int id = *((int*)arg);
    printf("Thread %d is running\nj",id);
    return NULL;
}

int main(){
    pthread_t threads[NUM_THREAD];
    int thread_ids[NUM_THREAD];


}