#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <pthread.h>
#include <sys/sem.h>
#include <sys/types.h>

/* declare union semun */
union semun {
    int val;
    struct semid_ds *buf;
    unsigned short  *array;
};

int semid; // semaphore id
pthread_t compute_thread, print_thread; // compute thread and print thread
int number = 0; // shared int value

/* P operation for semaphore */
void semaphore_p(int semid, int index) {
    struct sembuf ops = {
        .sem_num = index,
        .sem_op = -1,
        .sem_flg = 0
    };
    if (semop(semid, &ops, 1) == -1) {
        printf("semop failed: %s\n", strerror(errno));
        exit(0);
    }
}

/* V operation for semaphore */
void semaphore_v(int semid, int index) {
    struct sembuf ops = {
        .sem_num = index,
        .sem_op = 1,
        .sem_flg = 0
    };
    if (semop(semid, &ops, 1) == -1) {
        printf("semop failed: %s\n", strerror(errno));
        exit(0);
    }
}

/* compute thread */
void * compute(void * arg) {
    for (int index = 1 ; index <= 100 ; ++index) {
        semaphore_p(semid, 0); // acquire sem0 to prevent compute thread
        number = number + index;
        semaphore_v(semid, 1); // release sem1 to allow print thread
    }
    return NULL;
}

/* print thread */
void * print(void * arg) {
    for (int index = 1 ; index <= 100 ; ++index) {
        semaphore_p(semid, 1); // acquire sem1 to prevent print thread
        printf("current index: %d\t current_value: %d\n", index, number);
        semaphore_v(semid, 0); // release sem0 to allow compute thread
    }
    return NULL;
}

int main(void) {
    /* create the private semaphore set */
    /* size of set equals 2 */
    /* only user has read/write permission */
    if ((semid = semget(IPC_PRIVATE, 2, IPC_CREAT | 0600)) == -1) {
        printf("semget failed: cannot initialize sem set, %s\n", strerror(errno));
        exit(-1);
    }

    /* set initial values 1, 0 to semaphores */
    unsigned short values[2] = {1, 0};
    union semun sem_union = {.array = values};
    if (semctl(semid, 0, SETALL, sem_union) == -1) {
        printf("semctl failed: cannot set value for semaphore, %s\n", strerror(errno));
        exit(-1);
    }

    /* create threads */
    if ((pthread_create(&compute_thread, NULL, compute, NULL) == -1) || (pthread_create(&print_thread, NULL, print, NULL) == -1)) {
        printf("pthread_create failed: cannot create threads, %s\n", strerror(errno));
        exit(-1);
    }

    /* wait for threads to join */
    if (pthread_join(compute_thread, NULL) != 0 || pthread_join(print_thread, NULL) != 0) {
        printf("pthread_join failed\n");
        exit(-1);
    }

    /* release the semaphores */
    if (semctl(semid, 0, IPC_RMID, sem_union) == -1) {
        printf("semctl failed: cannot delete semaphore, %s\n", strerror(errno));
        exit(-1);
    }
    return 0;
}
