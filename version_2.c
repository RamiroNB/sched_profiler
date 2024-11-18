#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include <sched.h>
#include <string.h>

// Variáveis globais
char *buffer;
int buffer_size;
int current_index = 0;
sem_t semaphore;

// Função que cada thread executa
void* write_to_buffer(void *arg) {
    char character = *(char *)arg;

    while (1) {
        sem_wait(&semaphore); // Aguarda o semáforo

        if (current_index >= buffer_size) {
            sem_post(&semaphore); // Libera o semáforo
            break;
        }

        buffer[current_index] = character; // Escreve o caractere no buffer
        current_index++; // Incrementa o índice
        sem_post(&semaphore); // Libera o semáforo
    }

    return NULL;
}

// Função para pós-processamento do buffer
void post_process_buffer(int num_threads) {
    printf("Saída após pós-processamento:\n");
    char last_char = buffer[0];
    printf("%c", last_char);

    for (int i = 1; i < buffer_size; i++) {
        if (buffer[i] != last_char) {
            last_char = buffer[i];
            printf("%c", last_char);
        }
    }
    printf("\n");

    // Contar quantas vezes cada thread foi escalonada
    int counts[26] = {0};
    for (int i = 0; i < buffer_size; i++) {
        counts[buffer[i] - 'A']++;
    }

    for (int i = 0; i < num_threads; i++) {
        printf("%c = %d\n", 'A' + i, counts[i]);
    }
}

int main(int argc, char *argv[]) {
    if (argc != 4) {
        fprintf(stderr, "Uso: %s <tamanho_buffer> <num_threads> <policy>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    buffer_size = atoi(argv[1]);
    int num_threads = atoi(argv[2]);
    int policy = atoi(argv[3]);

    // Aloca o buffer global
    buffer = (char *)malloc(buffer_size * sizeof(char));
    if (buffer == NULL) {
        perror("Falha ao alocar o buffer");
        exit(EXIT_FAILURE);
    }

    pthread_t threads[num_threads];
    char thread_chars[num_threads];
    sem_init(&semaphore, 0, 1); // Inicializa o semáforo

    // Configurar a política de escalonamento
    struct sched_param param;
    param.sched_priority = 1; // Prioridade mínima para SCHED_FIFO e SCHED_RR

    switch (policy) {
        case 0: sched_setscheduler(0, SCHED_IDLE, &param); break;
        case 1: sched_setscheduler(0, SCHED_FIFO, &param); break;
        case 2: sched_setscheduler(0, SCHED_RR, &param); break;
        default: fprintf(stderr, "Política de escalonamento inválida\n"); exit(EXIT_FAILURE);
    }

    // Criação das threads
    for (int i = 0; i < num_threads; i++) {
        thread_chars[i] = 'A' + i;
        if (pthread_create(&threads[i], NULL, write_to_buffer, &thread_chars[i]) != 0) {
            perror("Falha ao criar thread");
            exit(EXIT_FAILURE);
        }
    }

    // Aguardar todas as threads terminarem
    for (int i = 0; i < num_threads; i++) {
        pthread_join(threads[i], NULL);
    }

    // Imprimir o buffer sem pós-processamento
    printf("Saída sem pós-processamento:\n");
    fwrite(buffer, sizeof(char), buffer_size, stdout);
    printf("\n");

    // Pós-processar o buffer
    post_process_buffer(num_threads);

    // Limpeza
    sem_destroy(&semaphore);
    free(buffer);

    return 0;
}

