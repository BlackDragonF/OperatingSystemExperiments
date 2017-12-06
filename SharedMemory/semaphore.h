#ifndef SEMAPHORE_H
#define SEMAPHORE_H

#include <sys/types.h>

#define MUTEX_LOCK 0
#define FULL_SLOTS 1
#define EMPTY_SLOTS 2

union semun {
    int val;
    struct semid_ds * buf;
    unsigned short * array;
};

int create_semaphore_set(int slots);
int retrieve_semaphore_set(key_t key);
void semaphore_p(int semid, int index);
void semaphore_v(int semid, int index);
void remove_semaphore_set(int semid);

#endif
