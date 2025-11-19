#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <unistd.h>

typedef struct {
    int value;
    pthread_mutex_t lock;
    pthread_cond_t cond;
} Semaphore;

void sem_init_custom(Semaphore *s, int value) {
    s->value = value;
    pthread_mutex_init(&s->lock, NULL);
    pthread_cond_init(&s->cond, NULL);
}

void sem_wait_custom(Semaphore *s) {
    pthread_mutex_lock(&s->lock);
    while (s->value == 0)
        pthread_cond_wait(&s->cond, &s->lock);
    s->value--;
    pthread_mutex_unlock(&s->lock);
}

void sem_signal_custom(Semaphore *s) {
    pthread_mutex_lock(&s->lock);
    s->value++;
    pthread_cond_signal(&s->cond);
    pthread_mutex_unlock(&s->lock);
}

/* -----------------------------------------------------
   PROBLEM 1: No-starve Readers–Writers
   ----------------------------------------------------- */

#define N_READERS 5
#define N_WRITERS 5

Semaphore roomEmpty;     // for writers
Semaphore turnstile;     // prevents starvation
Semaphore readSwitchLock;
int readerCount = 0;

void *reader1(void *arg) {
    int id = (long)arg;

    while (1) {
        sem_wait_custom(&turnstile);
        sem_signal_custom(&turnstile);

        sem_wait_custom(&readSwitchLock);
        readerCount++;
        if (readerCount == 1)
            sem_wait_custom(&roomEmpty);
        sem_signal_custom(&readSwitchLock);

        printf("Reader %d: reading\n", id);
        usleep(100000);

        sem_wait_custom(&readSwitchLock);
        readerCount--;
        if (readerCount == 0)
            sem_signal_custom(&roomEmpty);
        sem_signal_custom(&readSwitchLock);

        usleep(50000);
    }
    return NULL;
}

void *writer1(void *arg) {
    int id = (long)arg;

    while (1) {
        sem_wait_custom(&turnstile);
        sem_wait_custom(&roomEmpty);

        printf("Writer %d: writing\n", id);
        usleep(150000);

        sem_signal_custom(&roomEmpty);
        sem_signal_custom(&turnstile);

        usleep(100000);
    }
    return NULL;
}

void run_no_starve_readers_writers() {
    pthread_t r[N_READERS], w[N_WRITERS];

    sem_init_custom(&roomEmpty, 1);
    sem_init_custom(&turnstile, 1);
    sem_init_custom(&readSwitchLock, 1);

    for (long i = 0; i < N_READERS; i++) pthread_create(&r[i], NULL, reader1, (void*)i);
    for (long i = 0; i < N_WRITERS; i++) pthread_create(&w[i], NULL, writer1, (void*)i);

    for (int i = 0; i < N_READERS; i++) pthread_join(r[i], NULL);
    for (int i = 0; i < N_WRITERS; i++) pthread_join(w[i], NULL);
}

/* -----------------------------------------------------
   PROBLEM 2: Writer-Priority Readers–Writers
   ----------------------------------------------------- */

Semaphore wp_roomEmpty, wp_mutex, wp_turnstile;
int wp_readCount = 0;

void *reader2(void *arg) {
    int id = (long)arg;

    while (1) {
        sem_wait_custom(&wp_turnstile);
        sem_signal_custom(&wp_turnstile);

        sem_wait_custom(&wp_mutex);
        wp_readCount++;
        if (wp_readCount == 1)
            sem_wait_custom(&wp_roomEmpty);
        sem_signal_custom(&wp_mutex);

        printf("Reader %d (writer-priority): reading\n", id);
        usleep(90000);

        sem_wait_custom(&wp_mutex);
        wp_readCount--;
        if (wp_readCount == 0)
            sem_signal_custom(&wp_roomEmpty);
        sem_signal_custom(&wp_mutex);

        usleep(80000);
    }
    return NULL;
}

void *writer2(void *arg) {
    int id = (long)arg;

    while (1) {
        sem_wait_custom(&wp_turnstile);
        sem_wait_custom(&wp_roomEmpty);

        printf("Writer %d (writer-priority): writing\n", id);
        usleep(140000);

        sem_signal_custom(&wp_roomEmpty);
        sem_signal_custom(&wp_turnstile);

        usleep(90000);
    }
    return NULL;
}

void run_writer_priority_readers_writers() {
    pthread_t r[N_READERS], w[N_WRITERS];

    sem_init_custom(&wp_roomEmpty, 1);
    sem_init_custom(&wp_mutex, 1);
    sem_init_custom(&wp_turnstile, 1);

    for (long i = 0; i < N_READERS; i++) pthread_create(&r[i], NULL, reader2, (void*)i);
    for (long i = 0; i < N_WRITERS; i++) pthread_create(&w[i], NULL, writer2, (void*)i);

    for (int i = 0; i < N_READERS; i++) pthread_join(r[i], NULL);
    for (int i = 0; i < N_WRITERS; i++) pthread_join(w[i], NULL);
}

/* -----------------------------------------------------
   PROBLEM 3: Dining Philosophers #1
   ----------------------------------------------------- */

#define N_PHIL 5
Semaphore chopstick[N_PHIL];

void *philosopher1(void *arg) {
    int id = (long)arg;
    int left = id;
    int right = (id + 1) % N_PHIL;

    while (1) {
        printf("Philosopher %d: thinking\n", id);
        usleep(100000);

        int first = left < right ? left : right;
        int second = left < right ? right : left;

        sem_wait_custom(&chopstick[first]);
        sem_wait_custom(&chopstick[second]);

        printf("Philosopher %d: eating\n", id);
        usleep(100000);

        sem_signal_custom(&chopstick[first]);
        sem_signal_custom(&chopstick[second]);
    }
    return NULL;
}

void run_dining_philosophers_1() {
    pthread_t p[N_PHIL];

    for (int i = 0; i < N_PHIL; i++)
        sem_init_custom(&chopstick[i], 1);

    for (long i = 0; i < N_PHIL; i++)
        pthread_create(&p[i], NULL, philosopher1, (void*)i);

    for (int i = 0; i < N_PHIL; i++)
        pthread_join(p[i], NULL);
}

/* -----------------------------------------------------
   PROBLEM 4: Dining Philosophers #2
   ----------------------------------------------------- */

void *philosopher2(void *arg) {
    int id = (long)arg;
    int left = id;
    int right = (id + 1) % N_PHIL;

    while (1) {
        printf("Philosopher %d: thinking (asymmetric)\n", id);
        usleep(100000);

        if (id % 2 == 0) {
            sem_wait_custom(&chopstick[left]);
            sem_wait_custom(&chopstick[right]);
        } else {
            sem_wait_custom(&chopstick[right]);
            sem_wait_custom(&chopstick[left]);
        }

        printf("Philosopher %d: eating (asymmetric)\n", id);
        usleep(100000);

        sem_signal_custom(&chopstick[left]);
        sem_signal_custom(&chopstick[right]);
    }
}

void run_dining_philosophers_2() {
    pthread_t p[N_PHIL];

    for (int i = 0; i < N_PHIL; i++)
        sem_init_custom(&chopstick[i], 1);

    for (long i = 0; i < N_PHIL; i++)
        pthread_create(&p[i], NULL, philosopher2, (void*)i);

    for (int i = 0; i < N_PHIL; i++)
        pthread_join(p[i], NULL);
}

/* -----------------------------------------------------
   MAIN: Select problem from command line
   ----------------------------------------------------- */

int main(int argc, char *argv[]) {
    if (argc != 2) {
        printf("Usage: %s <problem number>\n", argv[0]);
        printf("1 = No-starve Readers–Writers\n");
        printf("2 = Writer-priority Readers–Writers\n");
        printf("3 = Dining Philosophers #1\n");
        printf("4 = Dining Philosophers #2\n");
        return 1;
    }

    int problem = atoi(argv[1]);
    switch (problem) {
        case 1: run_no_starve_readers_writers(); break;
        case 2: run_writer_priority_readers_writers(); break;
        case 3: run_dining_philosophers_1(); break;
        case 4: run_dining_philosophers_2(); break;
        default:
            printf("Invalid problem number.\n");
            return 1;
    }

    return 0;
}

