// include system headers
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include <sys/sem.h>
#include <sys/stat.h>

// include own header
#include "semaphore.h"

extern const char * __progname; // gcc defined as substitute for argv[0]

extern int verbose_flag; // verbose flag
extern key_t ipc_key; // IPC key

// private function list
int Semget(key_t key, int nsems, int semflg);

// semget wrapper
int Semget(key_t key, int nsems, int semflg) {
    int semid = -1;
    if ((semid = semget(key, nsems, semflg)) == -1) {
        printf("semget failed: %s.\n", strerror(errno));
        if (verbose_flag) printf("%s: failed to create semaphore set.\n", __progname);
    }
    return semid;
}

int create_semaphore_set(int slots) {
    int semid = -1;
    // use semget to create semaphore set with 3 semaphores
    // 0 - mutex lock, 1 - full slots, 2 - empty slots
    if ((semid = semget(ipc_key, 3, IPC_CREAT | IPC_EXCL | S_IRUSR | S_IWUSR)) == -1) {
        printf("semget failed: %s.\n", strerror(errno));
        if (verbose_flag) printf("%s: failed to create semaphore set.\n", __progname);
        exit(-1);
    }
    if (verbose_flag) printf("%s: semaphore set(2) created with id 0x%x.\n", __progname, semid);

    // use semctl to set value
    unsigned short values[3] = {1, 0, slots};
    union semun sem_val = {.array = values};
    if (semctl(semid, 0, SETALL, sem_val) == -1) {
        printf("semctl failed: %s.\n", strerror(errno));
        if (verbose_flag) printf("%s: failed to set values to semaphore set.\n", __progname);
        exit(-1);
    }
    if (verbose_flag) printf("%s: set 1 to semaphore 0(mutex), set 0 to semaphore 1(full slots), set %d to semaphore 2(empty slots).\n", __progname, slots);

    return semid;
}

int retrieve_semaphore_set(key_t key) {
    int semid = -1;
    // use semget to retrieve semaphore set
    if ((semid = semget(key, 3, 0)) == -1) {
        printf("semget failed: %s.\n", strerror(errno));
        if (verbose_flag) printf("%s: failed to retrieve semaphore set givn key 0x%x\n", __progname, key);
        exit(-1);
    }
    if (verbose_flag) printf("%s: semaphore set associated with key 0x%x retrieved.\n", __progname, key);
    
    return semid;
}

void semaphore_p(int semid, int index) {
    struct sembuf ops = {
        .sem_num = index,
        .sem_op = -1, // minus 1 for each P operation
        .sem_flg = 0
    };
    if (semop(semid, &ops, 1) == -1) {
        printf("semop failed: %s.\n", strerror(errno));
        if (verbose_flag) printf("%s: failed to operate P on semaphore %d.\n", __progname, index);
        exit(-1);
    }
}

void semaphore_v(int semid, int index) {
    struct sembuf ops = {
        .sem_num = index,
        .sem_op = 1, // add 1 for each V operation
        .sem_flg = 0
    };
    if (semop(semid, &ops, 1) == -1) {
        printf("semop failed: %s.\n", strerror(errno));
        if (verbose_flag) printf("%s: failed to operate V on semaphore %d.\n", __progname, index);
        exit(-1);
    }
}

void remove_semaphore_set(int semid) {
    union semun sem_val;
    // if (semctl(semid, 0, IPC_RMID, sem_val) == -1) {
    //     printf("semctl failed: %s.\n", strerror(errno));
    //     if (verbose_flag) printf("%s: failed to remove semaphore set.\n", __progname);
    //     exit(-1);
    // }
    // unnecessary to add printf for cleanup function
    semctl(semid, 0, IPC_RMID, sem_val);
}
