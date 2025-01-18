#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <time.h>

#define ERR(source) (perror(source), fprintf(stderr, "%s:%d\n", __FILE__, __LINE__), exit(EXIT_FAILURE))

typedef struct
{
    int thread_id;
} thread_arg;

void *thread_func(void *arg)
{
    thread_arg *targ = (thread_arg *)arg;
    printf("*\n");
    return NULL;
}

int main(int argc, char **argv)
{
    if(argc != 4)
    {
        fprintf(stderr, "Usage: %s, n, m, path\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    int n = atoi(argv[1]);
    int m = atoi(argv[2]);
    char *path = argv[3];

    if( n <= 0 || m <= 0)
    {
        fprintf(stderr, "n and m must be positive integers.\n");
        exit(EXIT_FAILURE);
    }

    FILE *file = fopen(path, "r");
    if(!file)
        ERR("fopen");

    char *line = NULL;
    size_t len = 0;
    if(getline(&line, &len, file) == -1)
    {
        free(line);
        fclose(file);
        ERR("getline");
    }

    printf("Header: %s", line);

    //rozmiar pliku
    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    fseek(file, strlen(line), SEEK_SET);
    long data_size = file_size - strlen(line);

    printf("File size: %ld bytes\n", file_size);
    printf("Data size (excluding header): %ld bytes\n", data_size);

    long chunk_size = data_size / m;
    printf("Chunk size: %ld bytes\n", chunk_size);

    pthread_t threads[n];
    thread_arg args[n];

    for(int i=0; i<n; i++)
    {
        args[i].thread_id = i;
        if(pthread_create(&threads[i], NULL, thread_func, &args[i]) != 0)
        {
            ERR("pthread_create");
        }
    }

    for(int i=0; i<n; i++)
    {
        if(pthread_join(threads[i], NULL) !=0)
        {
            ERR("pthread_join");
        }
    }

    free(line);
    fclose(file);

    return EXIT_SUCCESS;
}