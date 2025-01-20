#define _XOPEN_SOURCE 700
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <signal.h>
#include <unistd.h>

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

typedef struct{
    sigset_t *set;
    int value;
} thread_data_t;

// Funkcja wątku obsługującego sygnały
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

int main() {
    pthread_t sig_thread;
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

    // Oczekiwanie na zakończenie wątku obsługującego sygnały
    pthread_join(sig_thread, NULL);

    pthread_mutex_destroy(&mutex);

    return 0;
}
