#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <time.h>


#define NUM_THREADS 6

typedef struct{
    void* memory;
    size_t maxSize;
    size_t currSize;
    size_t front;
    size_t end;
    pthread_mutex_t lock;
    pthread_cond_t cv_prod;
    pthread_cond_t cv_cons;
    size_t sum;
} Queue;

typedef struct{
    Queue *q;
    int val;
    size_t thread_id;
} prod_args;

typedef struct{
    Queue *q;
    size_t thread_id;
} cons_args;

Queue* initialize(int len){
    Queue *q = (Queue *)malloc(sizeof(Queue));
    q->memory = malloc(sizeof(int)*len);
    q->maxSize = len;
    q->currSize = 0;
    q->front = 0;
    q->sum = 0;
    q->end = 0;
    pthread_mutex_init(&q->lock, NULL);
    pthread_cond_init(&q->cv_prod, NULL);
    pthread_cond_init(&q->cv_cons, NULL);
    return q;
}

void *push(void *arg){
    prod_args *args = (prod_args *)arg;
    Queue *q = args->q;
    int val = args->val;
    pthread_mutex_lock(&q->lock);
    if(q->currSize == q->maxSize){
        printf("Producer %d waiting\n", args->thread_id);
        pthread_cond_wait(&q->cv_prod, &q->lock);
        if(q->currSize == q->maxSize){
            printf("Producer %d woken up spuriously\n", args->thread_id);
        }
    }
    *((int *)q->memory + q->end) = val;
    q->end = (q->end + 1)%q->maxSize;
    q->currSize++;
     printf("Size of queue: %ld\n", q->currSize);
    pthread_cond_broadcast(&q->cv_cons);
    pthread_mutex_unlock(&q->lock);
    printf("Pushed value: %d\n", val);   
}

void *pop(void *arg){
    cons_args *args = (cons_args *)arg;
    Queue *q = args->q;
    pthread_mutex_lock(&q->lock);
    if(q->currSize == 0){
        printf("Consumer %d waiting\n", args->thread_id);
        pthread_cond_wait(&q->cv_cons, &q->lock);
        if(q->currSize == 0){
            printf("Consumer %d woken up spuriously\n", args->thread_id);
        }
    }
    int val = *((int *)q->memory + q->front);
    q->sum += val;
    q->front = (q->front+1)%q->maxSize;
    q->currSize--;
    //dummy computation keeps the thread busy
    // for(int i=0;i<1000000;i++){
    //     val = val*i + 1;
    // }
    pthread_cond_broadcast(&q->cv_prod);
    pthread_mutex_unlock(&q->lock);
    printf("Popped value: %d for thread %d\n", val, args->thread_id);
}

void compute(Queue *q){
    pthread_t threads[NUM_THREADS];
    prod_args args[NUM_THREADS];
    cons_args cargs[NUM_THREADS];
    for(int i=0;i<NUM_THREADS/2;i++){
        args[i].q = q;
        args[i].val = i;
        args[i].thread_id = i;
        pthread_create(&threads[i], NULL, push, (void *)&args[i]);
    }
    
    for(int i=NUM_THREADS/2;i<NUM_THREADS;i++){ 
        cargs[i].q = q;
        cargs[i].thread_id = i;
        pthread_create(&threads[i], NULL, pop, (void *)&cargs[i]);
    }
    for(int i=0;i<NUM_THREADS;i++){
        pthread_join(threads[i], NULL);
    }
}
int main(){
    Queue *q = initialize(1);
    // size of int is 4 bytes, so malloc size is 4*1000000 = 4MB
    // number of page faults = 4MB/4KB = 1024
    // stress test for 1 million elements and measure time to push and pop
    // start timer
    
    
    struct timespec start, end;
    clock_gettime(CLOCK_MONOTONIC, &start);
    
    compute(q);

    clock_gettime(CLOCK_MONOTONIC, &end);

    long seconds = end.tv_sec - start.tv_sec;
    long nanoseconds = end.tv_nsec - start.tv_nsec;

    // Handle nanosecond rollover
    if (nanoseconds < 0) {
        seconds -= 1;              // Borrow a second
        nanoseconds += 1000000000L; // Add 1 billion nanoseconds
    }

    // Convert to milliseconds
    double elapsed = (double)seconds * 1000.0 + (double)nanoseconds / 1e6;

    printf("Sum of elements: %ld\n", q->sum);
    printf("Elapsed time: %.5f ms\n", elapsed);

    free(q->memory);
    free(q);

    return 0;
}
