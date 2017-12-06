#ifndef EXPERIMENT1_H
#define EXPERIMENT1_H

#define BUFFER_SIZE 50

typedef void (*sighandler_t)(int);

void Pipe(int fildes[2]);
pid_t Fork(void);
void Signal(int signum, sighandler_t handler);
void Write(int fildes, const void * buf, size_t nbyte);
int Read(int fildes, void * buf, size_t nbyte);
void Kill(pid_t pid, int sig);
pid_t Waitpid(pid_t pid, int * stac_loc, int options);
void Close(int fildes);

#endif
