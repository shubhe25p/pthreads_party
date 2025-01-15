#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>

#define MAX_SIZE 3
#define NUM_THREADS 10
#define CACHE_LINE_SIZE 64

typedef struct {
    int *items;
    int front;
    int end;
    int cnt;
    pthread_mutex_t m;
    pthread_cond_t cv_prod;
    pthread_cond_t cv_cons;
    int waiting_cons;
    int waiting_prod;
} Queue;

typedef struct {
    Queue *q;
    int val;
} Args;

void* aligned_malloc(){
    void* raw_memory = malloc(MAX_SIZE+sizeof(void*)+CACHE_LINE_SIZE-1);
    uintptr_t aligned_address = ((uintptr_t)raw_memory + CACHE_LINE_SIZE - 1) & ~(CACHE_LINE_SIZE-1);
    *((void**)aligned_address-1) = raw_memory;
    return (void *)aligned_address;
}

void initialize(Queue *q){
    q->front = 0;
    q->end = 0;
    q->cnt=0;
    pthread_mutex_init(&q->m, NULL);
    pthread_cond_init(&q->cv_prod, NULL);
    pthread_cond_init(&q->cv_cons, NULL);
    q->items = (int *)aligned_malloc();
    q->waiting_cons=0;
    q->waiting_prod=0;
}
void *push(void *arg){
    Args *args = (Args *)arg;
    Queue *q = (Queue *)args->q;
    int val = args->val;
    pthread_mutex_lock(&q->m);
    if(q->waiting_prod>0 || q->cnt == MAX_SIZE){
        q->waiting_prod++;
        pthread_cond_wait(&q->cv_prod, &q->m);
        q->waiting_prod--;
    }

    q->items[q->end] = val;
    q->end = (q->end+1)%MAX_SIZE;
    q->cnt++;
    pthread_cond_broadcast(&q->cv_cons);
    pthread_mutex_unlock(&q->m);
    printf("Pushed value: %d\n", val);
}

void *pop(void *arg){
    Queue *q = (Queue *)arg;
    pthread_mutex_lock(&q->m);
    if(q->waiting_cons>0 || q->cnt==0){
        pthread_cond_wait(&q->cv_cons, &q->m);
    }
    int popVal = q->items[q->front];
    q->front = (q->front+1)%MAX_SIZE;
    q->cnt--;
    pthread_cond_broadcast(&q->cv_prod);
    pthread_mutex_unlock(&q->m);
    printf("Popped value: %d\n", popVal);
}

int main(){
    Queue q;
    initialize(&q);
    pthread_t threads[NUM_THREADS];
    for(int i=0; i<NUM_THREADS; i++){
        if(i%2==0){
            Args *args = (Args *)malloc(sizeof(Args));
            args->q = &q;
            args->val = i;
            pthread_create(&threads[i], NULL, push, args);
        }else{  
            pthread_create(&threads[i], NULL, pop, &q);
        }
    }

    // join
    for(int i=0; i<NUM_THREADS; i++){
        pthread_join(threads[i], NULL);
    }


    
    return 0;
}
