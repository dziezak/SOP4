#define _XOPEN_SOURCE 700
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <signal.h>
#include <unistd.h>
#include <errno.h>

#define NUM_THREADS 1000
#define ARRAY_SIZE 10
#define WAIT_TIMEOUT_SECONDS 2

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond = PTHREAD_COND_INITIALIZER;
int array[ARRAY_SIZE];
int counter = 0;

typedef struct{
    sigset_t *set;
    int value;
} thread_data_t;

void *signal_handler_thread(void *arg) {
    thread_data_t *data = (thread_data_t *)arg;
    sigset_t *set = data->set;
    int sig;

    while (1) {
        sigwait(set, &sig);
        pthread_mutex_lock(&mutex);
        printf("Received signal: %d\n", sig);
        data->value += 10;
        printf("Thread recived value and incresed it by 10 = %d\n", data->value);
        if (sig == SIGINT) {
            printf("SIGINT received, exiting signal handler thread.\n");
            pthread_mutex_unlock(&mutex);
            pthread_exit(NULL);
        }
    }
    pthread_mutex_unlock(&mutex);
    return NULL;
}

void *thread_func(void* arg){
    long thread_id = (long)arg;
    int index = thread_id % ARRAY_SIZE;

    srand(time(NULL) + thread_id);
    int increment = rand() % 10 + 1;
    index = (index + increment)%ARRAY_SIZE;

    pthread_mutex_lock(&mutex);
    array[index]++;
    counter++;
    printf("Therad with ID %ld incremented value at index %d to %d\n", thread_id, index, array[index]);
    printf("Current counter value: %d\n", counter);
    pthread_cond_signal(&cond);
    pthread_mutex_unlock(&mutex);
    return NULL;
}

void *waiting_thread_func(void *arg){
    pthread_mutex_lock(&mutex);
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    ts.tv_sec += WAIT_TIMEOUT_SECONDS;

    printf("Thread waiting for counter to exceed 50...\n");
    long thread_id = (long)arg;
    int index = thread_id % ARRAY_SIZE;

    while(array[index] < 50){
        int ret = pthread_cond_timedwait(&cond, &mutex, &ts);
        if(ret == ETIMEDOUT){
            printf("Thread timed out waiting for the counter to exceed 10.\n");
            pthread_mutex_unlock(&mutex);
            return NULL;
        }
    }

    printf("Thread detected counter exceeding 10.\n");
    //kill(getpid(), SIGINT);
    pthread_mutex_unlock(&mutex);
    return NULL;
}

void array_Print(){
    printf("Final array values:\n");
    for(int i=0; i<ARRAY_SIZE; i++)
        printf("array[%d] = %d\n", i, array[i]);
}

int main() {
    pthread_t sig_thread;
    pthread_t threads[NUM_THREADS];
    pthread_t waiting_threads[ARRAY_SIZE];
    sigset_t set;
    thread_data_t data;

    // Blokowanie sygnałów w głównym wątku
    sigemptyset(&set);
    sigaddset(&set, SIGINT);
    sigaddset(&set, SIGTERM);
    pthread_sigmask(SIG_BLOCK, &set, NULL);

    data.set = &set;
    data.value = 2127;

    // Tworzenie wątku obsługującego sygnały
    pthread_create(&sig_thread, NULL, signal_handler_thread, (void *)&data);

    for(int i=0; i<NUM_THREADS; i++)
        pthread_create(&threads[i], NULL, thread_func, (void*)i);

    for(int i=0; i<ARRAY_SIZE; i++)
        pthread_create(&waiting_threads[i], NULL, waiting_thread_func, NULL);

    for(int i=0; i<NUM_THREADS; i++)
        pthread_join(threads[i], NULL);

    for(int i=0; i<ARRAY_SIZE; i++)
        pthread_join(waiting_threads[i], NULL);

    // Oczekiwanie na zakończenie wątku obsługującego sygnały
    pthread_join(sig_thread, NULL);

    array_Print();

    pthread_mutex_destroy(&mutex);
    pthread_cond_destroy(&cond);

    return 0;
}