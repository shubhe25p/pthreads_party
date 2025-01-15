#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/wait.h>
#include <pthread.h>
#include <string.h>
#include <sys/stat.h>

#define SHM_NAME "/shared_queue"
#define QUEUE_SIZE 10
#define NUM_PRODUCERS 2
#define NUM_CONSUMERS 4

typedef struct {
    int items[QUEUE_SIZE];
    pthread_mutex_t mutex;
    pthread_cond_t cv_prod;
    pthread_cond_t cv_cons;
    int front;
    int end;
    int cnt;
    int capacity;
    int waiting_cons;
    int waiting_prod;
} SharedQueue;

typedef struct {
    SharedQueue* queue;
    int data;
} Args;

void init_queue(SharedQueue* queue) {
    pthread_mutexattr_t mutex_attr;
    pthread_mutexattr_init(&mutex_attr);
    pthread_mutexattr_setpshared(&mutex_attr, PTHREAD_PROCESS_SHARED);
    pthread_mutex_init(&queue->mutex, &mutex_attr);

    pthread_condattr_t cond_attr;
    pthread_condattr_init(&cond_attr);
    pthread_condattr_setpshared(&cond_attr, PTHREAD_PROCESS_SHARED);
    pthread_cond_init(&queue->cv_prod, &cond_attr);
    pthread_cond_init(&queue->cv_cons, &cond_attr);

    queue->front = 0;
    queue->end = 0;
    queue->cnt = 0;
    queue->waiting_cons = 0;
    queue->waiting_prod = 0;
    queue->capacity = QUEUE_SIZE;
}

void *push(void *arg) {
    Args *args = (Args *)arg;
    SharedQueue *q = args->queue;
    int val = args->data;
    pthread_mutex_lock(&q->mutex);
    
    while (q->waiting_prod > 0 || q->cnt == q->capacity) {
        q->waiting_prod++;
        pthread_cond_wait(&q->cv_prod, &q->mutex);
        q->waiting_prod--;
    }
    q->items[q->end] = val;
    q->end = (q->end + 1) % q->capacity;
    q->cnt++;
    pthread_cond_signal(&q->cv_cons);
    pthread_mutex_unlock(&q->mutex);
    printf("Pushed value: %d\n",val);
}

void *pop(void *arg) {
    SharedQueue *q = (SharedQueue *)arg;
    pthread_mutex_lock(&q->mutex);
    
    while (q->waiting_cons > 0 || q->cnt == 0) {
        q->waiting_cons++;
        pthread_cond_wait(&q->cv_cons, &q->mutex);
        q->waiting_cons--;
    }
    
    int popval = q->items[q->front];
    q->front = (q->front + 1) % q->capacity;
    q->cnt--;
    
    pthread_cond_signal(&q->cv_prod);
    pthread_mutex_unlock(&q->mutex);
    
    printf("Popped value: %d\n",popval);
}

void producer(SharedQueue* queue) {
    pthread_t tid[NUM_PRODUCERS];

    for (int i = 0; i < QUEUE_SIZE; i++) {
        Args *args = (Args *)malloc(sizeof(Args));
        args->queue = queue;
        args->data = 100 + i;
        pthread_create(&tid[i%NUM_PRODUCERS], NULL, push, args);
    }
    
    
}

void consumer(SharedQueue* queue) {
    pthread_t tid[NUM_CONSUMERS];
    for (int i = 0; i < QUEUE_SIZE; i++) {
        pthread_create(&tid[i%NUM_CONSUMERS], NULL, pop, queue);
        
    }

   
}

int main() {
    int fd = shm_open(SHM_NAME, O_CREAT | O_RDWR, 0666);
    if (fd == -1) {
        perror("shm_open");
        exit(1);
    }

    if (ftruncate(fd, sizeof(SharedQueue)) == -1) {
        perror("ftruncate");
        exit(1);
    }

    SharedQueue* queue = mmap(NULL, sizeof(SharedQueue), PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (queue == MAP_FAILED) {
        perror("mmap");
        exit(1);
    }

    init_queue(queue);

    pid_t pid1, pid2;

    pid1 = fork();
    if (pid1 == 0) {
        // Child process 1 (Producer)
        producer(queue);
        exit(0);
    }

    pid2 = fork();
    if (pid2 == 0) {
        // Child process 2 (Consumer)
        consumer(queue);
        exit(0);
    }

    // Wait for child processes to finish
    waitpid(pid1, NULL, 0);
    waitpid(pid2, NULL, 0);

    // Cleanup
    pthread_mutex_destroy(&queue->mutex);
    pthread_cond_destroy(&queue->cv_prod);
    pthread_cond_destroy(&queue->cv_cons);
    munmap(queue, sizeof(SharedQueue));
    close(fd);
    shm_unlink(SHM_NAME);

    return 0;
}
