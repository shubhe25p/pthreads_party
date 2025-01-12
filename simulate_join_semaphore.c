#include <semaphore.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
typedef struct shared_data{
    sem_t semaphore;
    pthread_mutex_t lock;
    int ref_count;
    int data;
} shared_data_t;

void state_init(shared_data_t *shared_data){
    sem_init(&shared_data->semaphore, 0, 0); // sem shared between threads of a process
    pthread_mutex_init(&shared_data->lock, 0);
    shared_data->ref_count = 2;
    shared_data->data = -1;
}
int wait_for_other_thread(shared_data_t *shared_data){
    sem_wait(&shared_data->semaphore);
    int data = shared_data->data;
    pthread_mutex_lock(&shared_data->lock);
    int ref_count = --shared_data->ref_count;
    pthread_mutex_unlock(&shared_data->lock);
    if(ref_count==0)
        free(shared_data);
    return data;
}

void *save_data(void *shared_pg){
    shared_data_t *shared_data = (shared_data_t *)shared_pg;
    shared_data->data = 162;
    sem_post(&shared_data->semaphore);
    pthread_mutex_lock(&shared_data->lock);
    int ref_count = --shared_data->ref_count;
    pthread_mutex_unlock(&shared_data->lock);
    if(ref_count==0)
            free(shared_data);
    pthread_exit(NULL);
}

int main(){
    shared_data_t *shared_data = (shared_data_t* )malloc(sizeof(shared_data_t));
    state_init(shared_data);
    pthread_t tid;
    int err = pthread_create(&tid, NULL, save_data, shared_data);
    if(err !=0){
        printf("Thread cannot be created\n");
        return 0;
    }
    int data = wait_for_other_thread(shared_data);
    printf("Data in Parent: %d\n", data);
    return 0;
}
