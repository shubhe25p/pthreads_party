#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

struct thread_info{
    pthread_t tid;
    int thread_num;
};
pthread_mutex_t m = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t c = PTHREAD_COND_INITIALIZER;

static int sh_arr[10];
static void* start_routine(void *arg){
     
    struct thread_info *tinfo = arg;
    sleep(2);
    printf("Thread %d is created here \n",tinfo->thread_num);
    pthread_mutex_lock(&m);
    sh_arr[tinfo->thread_num] = 1;
    pthread_cond_signal(&c);
    pthread_mutex_unlock(&m);
}

void pthr_join(int tid){
    
    pthread_mutex_lock(&m);
    while(!sh_arr[tid]){
        pthread_cond_wait(&c, &m);
    }
    pthread_mutex_unlock(&m);
}
int main(){
    int s;
    printf("Master thread created here\n");
    struct thread_info *tinfo;
    tinfo = malloc(10*sizeof(struct thread_info));
    //create thread
    for(int i=0;i<10;i++)
    {
        tinfo[i].thread_num = i;
        s = pthread_create(&(tinfo[i].tid), NULL, start_routine, &tinfo[i]);
    }
    //simulate join thread with condition vars
    for(int i=0;i<10;i++)
    {
        printf("Master thread busy waits here\n");
        pthr_join(tinfo[i].thread_num);
    }

        
    free(tinfo);
    return 0;
}
