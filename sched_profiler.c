#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include <string.h>
#include <sched.h>

// Declaração de variáveis globais
char *buffer;
char *buffer_start;
int buffer_size;
sem_t semaphore;
int *thread_counts;

void *thread_function(void *arg) {
    char c = *(char *)arg;
    while (1) {
        sem_wait(&semaphore); // Aguarda liberação do semáforo
        
        if (buffer - buffer_start >= buffer_size) {
            sem_post(&semaphore); // Libera o semáforo
            break; // Sai do loop se o buffer estiver cheio
        }
        
        *buffer = c; // Escreve no buffer
        buffer++; // Atualiza o ponteiro do buffer
        thread_counts[c - 'A']++; // Incrementa contador para a thread
        
        sem_post(&semaphore); // Libera o semáforo
    }
    return NULL;
}

void process_buffer(int num_threads) {
    printf("Saída após pós-processamento:\n");
    char last_char = buffer_start[0];
    printf("%c", last_char);
    for (int i = 1; i < buffer_size; i++) {
        if (buffer_start[i] != last_char) {
            last_char = buffer_start[i];
            printf("%c", last_char);
        }
    }
    printf("\n");
    
    // Imprime conte contadores
    for (int i = 0; i < num_threads; i++) {
        printf("%c = %d\n", 'A' + i, thread_counts[i]);
    }
}

int main(int argc, char *argv[]) {
    if (argc != 4) {
        fprintf(stderr, "Uso: %s <tamanho_buffer> <num_threads> <policy>\n", argv[0]);
        return EXIT_FAILURE;
    }

    buffer_size = atoi(argv[1]);
    int num_threads = atoi(argv[2]);
    char* policy_str = argv[3];
    int policy;

    if (strcmp(policy_str, "SCHED_LOW_IDLE") == 0) {
        // Defina o valor adequado para sua plataforma
    } else if (strcmp(policy_str, "SCHED_IDLE") == 0) {
        // Defina o valor adequado para sua plataforma
    } else if (strcmp(policy_str, "SCHED_FIFO") == 0) {
        policy = SCHED_FIFO;
    } else if (strcmp(policy_str, "SCHED_RR") == 0) {
        policy = SCHED_RR;
    } else {
        fprintf(stderr, "Política de escalonamento desconhecida: %s\n", policy_str);
        return EXIT_FAILURE;
    }
    
    buffer = (char *)malloc(buffer_size * sizeof(char));
    buffer_start = buffer;
    sem_init(&semaphore, 0, 1);
    thread_counts = (int *)calloc(num_threads, sizeof(int));

    pthread_t threads[num_threads];
    char thread_args[num_threads];

    for (int i = 0; i < num_threads; i++) {
        thread_args[i] = 'A' + i;
        struct sched_param param;
        param.sched_priority = (policy == SCHED_FIFO || policy == SCHED_RR) ? 1 : 0;
        pthread_attr_t attr;
        
        pthread_attr_init(&attr);
        pthread_attr_setschedpolicy(&attr, policy);
        pthread_attr_setschedparam(&attr, &param);
        
        if (pthread_create(&threads[i], &attr, thread_function, &thread_args[i]) != 0) {
            perror("Erro ao criar thread");
            return EXIT_FAILURE;
        }
    }

    for (int i = 0; i < num_threads; i++) {
        pthread_join(threads[i], NULL);
    }

    printf("Saída sem pós-processamento:\n");
    printf("%s\n", buffer_start);

    process_buffer(num_threads);

    sem_destroy(&semaphore);
    free(buffer_start);
    free(thread_counts);

    return EXIT_SUCCESS;
}
