// include system headers
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <errno.h>
#include <string.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/shm.h>

// include own header
#include "ring-buffer.h"

extern const char * __progname; // gcc defined as substitute for argv[0]

extern int verbose_flag; // verbose flag
extern key_t ipc_key; // IPC key

extern int type; // implementation type
extern int buffer_capacity; // buffer capacity
extern int buffer_number; // buffer number

// private function list, all wrapper functions
int Shmget(key_t key, size_t size, int shmflg);
void * Shmat(int shmid, const void * shmaddr, int shmflg);
void Shmdt(const void * shmaddr);

// shmget wrapper
int Shmget(key_t key, size_t size, int shmflg) {
    int shmid = -1;
    if ((shmid = shmget(key, size, shmflg)) == -1) {
        printf("shmget failed: %s.\n", strerror(errno));
        if (verbose_flag) printf("%s: failed to apply for shared memory segment.\n", __progname);
        exit(-1);
    }
    return shmid;
}

// shmat wrapper
void * Shmat(int shmid, const void * shmaddr, int shmflg) {
    void * ptr = (void *)(-1);
    if ((intptr_t)(ptr = shmat(shmid, shmaddr, shmflg)) == -1) {
        printf("shmat failed: %s.\n", strerror(errno));
        if (verbose_flag) printf("%s: failed to attach shared memory segment to current address space.\n", __progname);
        exit(-1);
    }
    return ptr;
}

//shmdt wrapper
void Shmdt(const void * shmaddr) {
    if (shmdt(shmaddr) == -1) {
        printf("shmdt failed: %s.\n", strerror(errno));
        if (verbose_flag) printf("%s: failed to deattach shared memory segment.\n", __progname);
    }
}

// create ring buffer and return its shared memory id
int create_ring_buffer(void) {
    // size for ring buffer to be created
    size_t size = sizeof(struct RingBuffer) + (sizeof(struct BufferEntry) + buffer_capacity) * buffer_number;
    // apply for shared memory segment
    int shmid = Shmget(ipc_key, size, IPC_CREAT | S_IRUSR | S_IWUSR);
    if (verbose_flag) printf("%s: shared memory for ring buffer applied.\n", __progname);

    // attach ring buffer(shmid(int) -> ring_buffer(struct RingBuffer *))
    struct RingBuffer * buffer = Shmat(shmid, NULL, SHM_R | SHM_W);
    if (verbose_flag) printf("%s: ring buffer attached.\n", __progname);

    // set default in/out
    buffer->in = 0;
    buffer->out = 0;

    if (verbose_flag) printf("%s: ring buffer initialized.\n", __progname);

    // deattach ring buffer
    Shmdt(buffer);

    return shmid;
}

int retrieve_ring_buffer(key_t key) {
    // size for ring buffer to be retrieved
    size_t size = sizeof(struct RingBuffer) + (sizeof(struct BufferEntry) + buffer_capacity) * buffer_number;
    // retrieve existed shared memory segment
    int shmid = Shmget(key, size, 0);
    if (verbose_flag) printf("%s: ring buffer associated with key 0x%x retrieved.\n", __progname, key);

    return shmid;
}

struct RingBuffer * attach_ring_buffer(int shmid) {
    // attach ring buffer
    struct RingBuffer * ring_buffer = Shmat(shmid, NULL, SHM_W | SHM_R);
    if (verbose_flag) printf("%s: attached ring buffer to current address space.\n", __progname);

    return ring_buffer;
}

void deattach_ring_buffer(struct RingBuffer * ring_buffer) {
    // deattach ring buffer
    Shmdt(ring_buffer);
}

void produce(struct RingBuffer * ring_buffer, int size, void * buffer) {
    // version 3 - retry loop
    if (type == 2) while(ring_buffer->in - ring_buffer->out == buffer_number);

    // get next in buffer entry
    struct BufferEntry * entry = (struct BufferEntry *)((char *)(ring_buffer + 1) + (sizeof(struct BufferEntry) + buffer_capacity) * (ring_buffer->in % buffer_number));
    // copy from temp buffer to shared memory buffer and set size
    memcpy(entry->bytes, buffer, size);
    entry->size = size;
    if (verbose_flag) printf("%s: %d bytes data produced into buffer %d.\n", __progname, size, (ring_buffer->in % buffer_number));
    // modify in
    ring_buffer->in = ring_buffer->in + 1;
}

void consume(struct RingBuffer * ring_buffer, int * size, void * buffer) {
    // version 3 - retry loop
    if (type == 2) while (ring_buffer->in - ring_buffer->out == 0);

    // get next out buffer entry
    struct BufferEntry * entry = (struct BufferEntry *)((char *)(ring_buffer + 1) + (sizeof(struct BufferEntry) + buffer_capacity) * (ring_buffer->out % buffer_number));
    // get size from buffer entry
    *size = entry->size;
    // copy from shared memory buffer to temp buffer
    memcpy(buffer, entry->bytes, *size);
    entry->size = 0;
    if (verbose_flag) printf("%s: %d bytes data consumed from buffer %d.\n", __progname, *size, (ring_buffer->out % buffer_number));
    // modity out and clean size
    ring_buffer->out = ring_buffer->out + 1;
}

void delete_ring_buffer(int shmid) {
    // if (shmctl(shmid, IPC_RMID, NULL) == -1) {
    //     printf("shmctl failed: %s.\n", strerror(errno));
    //     if (verbose_flag) printf("%s: failed to delete ring buffer.\n", __progname);
    //     exit(-1);
    // }
    // unnecessary to add printf for cleanup function
    shmctl(shmid, IPC_RMID, NULL);
}
