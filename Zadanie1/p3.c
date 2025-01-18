#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>

#define ERR(source) (perror(source), fprintf(stderr, "%s:%d\n", __FILE__, __LINE__), exit(EXIT_FAILURE))

typedef struct node {
    char *line;
    struct node *prev;
    struct node *next;
} node;

typedef struct {
    int thread_id; 
    long start;    
    long size;     
} thread_arg;

typedef struct {
    long start;
    long size;
    int is_taken;
} chunk;

pthread_mutex_t chunk_mutex = PTHREAD_MUTEX_INITIALIZER;
chunk *chunks;
int chunks_remaining;
node *lists[100]; 
pthread_mutex_t lists_mutex[100]; 
char* PATH;

void append_to_list(node **head, char *line) {
    node *new_node = malloc(sizeof(node));
    if (!new_node) ERR("malloc");
    new_node->line = strdup(line);
    if (!new_node->line) ERR("strdup");
    new_node->next = NULL;
    if (!*head) {
        new_node->prev = NULL;
        *head = new_node;
    } else {
        node *tail = *head;
        while (tail->next) tail = tail->next;
        tail->next = new_node;
        new_node->prev = tail;
    }
}

void *thread_func(void *arg) {
    thread_arg *targ = (thread_arg *)arg;
    FILE *file = fopen(PATH, "r"); 
    if (!file) ERR("fopen");

    while (1) {
        pthread_mutex_lock(&chunk_mutex);
        chunk *assigned_chunk = NULL;
        for (int i = 0; i < chunks_remaining; i++) {
            if (!chunks[i].is_taken) {
                chunks[i].is_taken = 1;
                assigned_chunk = &chunks[i];
                break;
            }
        }
        pthread_mutex_unlock(&chunk_mutex);

        if (!assigned_chunk) {
            break;
        }

        char buffer[1024];
        fseek(file, assigned_chunk->start, SEEK_SET);
        long bytes_to_read = assigned_chunk->size;

        pthread_mutex_lock(&lists_mutex[targ->thread_id]);
        while (bytes_to_read > 0 && fgets(buffer, sizeof(buffer), file)) {
            long len = strlen(buffer);
            if (len > bytes_to_read) {
                buffer[bytes_to_read] = '\0';
            }
            append_to_list(&lists[targ->thread_id], buffer);
            bytes_to_read -= len;
        }
        pthread_mutex_unlock(&lists_mutex[targ->thread_id]);
    }

    fclose(file);
    return NULL;
}

int main(int argc, char **argv) {
    if (argc != 4) {
        fprintf(stderr, "Usage: %s n m path\n", argv[0]);
        exit(EXIT_FAILURE);
    }
    PATH = argv[0];

    int n = atoi(argv[1]);
    int m = atoi(argv[2]);
    char *path = argv[3];

    if (n <= 0 || m <= 0) {
        fprintf(stderr, "n and m must be positive integers.\n");
        exit(EXIT_FAILURE);
    }

    FILE *file = fopen(path, "r");
    if (!file) {
        ERR("fopen");
    }

    char *line = NULL;
    size_t len = 0;
    if (getline(&line, &len, file) == -1) {
        free(line);
        fclose(file);
        ERR("getline");
    }

    printf("Header: %s", line);

    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    fseek(file, strlen(line), SEEK_SET);
    long data_size = file_size - strlen(line);

    printf("File size: %ld bytes\n", file_size);
    printf("Data size (excluding header): %ld bytes\n", data_size);

    long chunk_size = data_size / m;
    long remainder = data_size % m;

    chunks = malloc(m * sizeof(chunk));
    if (!chunks) {
        ERR("malloc");
    }

    long current_start = ftell(file);
    for (int i = 0; i < m; i++) {
        chunks[i].start = current_start;
        chunks[i].size = chunk_size + (i < remainder ? 1 : 0);
        chunks[i].is_taken = 0;
        current_start += chunks[i].size;
    }

    chunks_remaining = m;

    for (int i = 0; i < n; i++) {
        lists[i] = NULL;
        if (pthread_mutex_init(&lists_mutex[i], NULL) != 0) {
            ERR("pthread_mutex_init");
        }
    }

    pthread_t threads[n];
    thread_arg args[n];

    for (int i = 0; i < n; i++) {
        args[i].thread_id = i;
        if (pthread_create(&threads[i], NULL, thread_func, &args[i]) != 0) {
            ERR("pthread_create");
        }
    }

    for (int i = 0; i < n; i++) {
        if (pthread_join(threads[i], NULL) != 0) {
            ERR("pthread_join");
        }
    }

    for (int i = 0; i < n; i++) {
        pthread_mutex_lock(&lists_mutex[i]);
        node *current = lists[i];
        while (current) {
            printf("Thread %d: %s\n", i, current->line);
            current = current->next;
        }
        pthread_mutex_unlock(&lists_mutex[i]);
    }

    for (int i = 0; i < n; i++) {
        pthread_mutex_destroy(&lists_mutex[i]);
        node *current = lists[i];
        while (current) {
            node *tmp = current;
            current = current->next;
            free(tmp->line);
            free(tmp);
        }
    }

    free(chunks);
    free(line);
    fclose(file);

    return EXIT_SUCCESS;
}
