#ifndef RING_BUFFER_H
#define RING_BUFFER_H

#include <sys/types.h> // contains definition for key_t

// shared memory generally structs below:
//
//  <---RingBuffer---> <-------BufferEntry0-------> <-------
// +--------+---------+--------+-------------------+--------+--
// |   in   |   out   |  size  |       bytes       |  size  |...
// +--------+---------+--------+-------------------+--------+--
//
// RingBuffer contains two BufferEntry pointers in/out
// BufferEntry contains size and bytes, where size indicates actual bytes stored in buffer
// bytes is stored data, implemented with 0 length array

// struct RingBuffer and struct BufferEntry should be private actually
// TO BE FIXED
// ring buffer definition
struct RingBuffer {
    int in;
    int out;
};

struct BufferEntry {
    int size;
    char bytes[0];
};

// create/retrieve/delete
int create_ring_buffer(void);
int retrieve_ring_buffer(key_t key);
void delete_ring_buffer(int shmid);

// attach and deattach
struct RingBuffer * attach_ring_buffer(int shmid);
void deattach_ring_buffer(struct RingBuffer * ring_buffer);

// produce and consume
void produce(struct RingBuffer * ring_buffer, int size, void * buffer);
void consume(struct RingBuffer * ring_buffer, int * size, void * buffer);

#endif
