// include system headers
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>

// include user headers
#include "semaphore.h"
#include "ring-buffer.h"

extern const char * __progname; // gcc defined as substitute for argv[0]

int verbose_flag = 0; // verbose flag
key_t ipc_key; // IPC key

int type; // implementation type
int buffer_capacity, buffer_number; // buffer capacity and number
const char * file; // source file name

// private function list
ssize_t Read(int fildes, void * buf, size_t nbyte);

// read wrapper
ssize_t Read(int fildes, void * buf, size_t nbyte) {
    int result = -1;
    if ((result = read(fildes, buf, nbyte)) == -1) {
        printf("read failed: %s.\n", strerror(errno));
        if (verbose_flag) printf("%s: failed to read from file %s.\n", __progname, file);
        exit(-1);
    }
    return result;
}

// program entry
int main(int argc, char * argv[]) {
    // argument validation and casting
    if (argc != 7) {
        printf("%s: wrong argument number!\n", argv[0]);
        exit(-1);
    }
    verbose_flag = atoi(argv[1]);
    ipc_key = atoi(argv[2]);
    type = atoi(argv[3]);
    buffer_capacity = atoi(argv[4]);
    buffer_number = atoi(argv[5]);
    file = argv[6];

    // retrieve semaphore set
    int semid = retrieve_semaphore_set(ipc_key);

    // retrieve ring buffer
    int shmid = retrieve_ring_buffer(ipc_key);
    struct RingBuffer * ring_buffer = attach_ring_buffer(shmid);

    // open write file
    int fd = 0;
    if ((fd = open(file, O_RDONLY, S_IRUSR | S_IWUSR)) == -1) {
        printf("open failed: %s.\n", strerror(errno));
        if (verbose_flag) printf("%s: failed to open file %s for reading.\n", argv[0], file);
        exit(-1);
    }
    if (verbose_flag) printf("%s: file %s opened.\n", argv[0], file);

    // put bytes to shared ring buffer
    int byte_count; // byte number that read from file
    char bytes[buffer_capacity]; // temp buffer
    int end_of_file_flag = 0; // mark end of file
    do { // read from file to temp buffer
        if ((byte_count = Read(fd, bytes, buffer_capacity)) == 0) end_of_file_flag = 1;
        if (verbose_flag) printf("%s: %d bytes read from file.\n", argv[0], byte_count);
        // version 1 & 2 - (mutex)/semaphores implementation
        if (type == 1) semaphore_p(semid, EMPTY_SLOTS); // empty slots minus 1
        // semaphore_p(semid, MUTEX_LOCK); // mutex lock acquired
        produce(ring_buffer, byte_count, bytes);
        // semaphore_v(semid, MUTEX_LOCK); // mutex lock released
        // unnecessary for mutex, cuz single producer and single consumer won't read / write
        // same buffer at the same time
        if (type == 1) semaphore_v(semid, FULL_SLOTS); // full slots add 1

    } while (!end_of_file_flag);
    if (verbose_flag) printf("%s: put process succeeded.\n", __progname);

    // clean up
    close(fd);
    deattach_ring_buffer(ring_buffer);

    return 0;
}
