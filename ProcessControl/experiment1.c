#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/wait.h>

#include "experiment1.h"

int fildes[2]; // pipe file descriptior
pid_t write_process = 0, read_process = 0; // child process ids
volatile sig_atomic_t signal_flag = 0; // global flag

/* signal handlers */
void parent_sigint_handler(int sig) {
    signal_flag = 1;
}

void child_sigusr1_handler(int sig) {
    signal_flag = 1;
}

/* wrapper functions */
void Pipe(int fildes[2]) {
    if (pipe(fildes) == -1) {
        printf("pipe failed: %s\n", strerror(errno));
        exit(-1);
    }
}

void Close(int fildes) {
    if (close(fildes) == -1) {
        printf("close failed: %s\n", strerror(errno));
        exit(-1);
    }
}

void Write(int fildes, const void * buf, size_t nbyte) {
    if (write(fildes, buf, nbyte) == -1) {
        printf("write failed: %s\n", strerror(errno));
        exit(-1);
    }
}

int Read(int fildes, void * buf, size_t nbyte) {
    int rbyte = read(fildes, buf, nbyte);
    if (rbyte == -1 && errno != EAGAIN) {
        printf("read failed: %s\n", strerror(errno));
        exit(-1);
    }
    return rbyte;
}

pid_t Fork(void) {
    pid_t pid = fork();
    if (pid == -1) {
        printf("fork failed: %s\n", strerror(errno));
        exit(-1);
    }
    return pid;
}

void Signal(int signum, sighandler_t handler) {
    if (signal(signum, handler) == SIG_ERR) {
        printf("signal failed: %s\n", strerror(errno));
        exit(-1);
    }
}

void Kill(pid_t pid, int sig) {
    if (kill(pid, sig) == -1) {
        printf("kill failed: %s\n", strerror(errno));
        exit(-1);
    }
}

pid_t Waitpid(pid_t pid, int * stac_loc, int options) {
    pid_t return_pid = waitpid(pid, stac_loc, options);
    if (return_pid == -1) {
        printf("waitpid failed: %s\n", strerror(errno));
        exit(-1);
    }
    return return_pid;
}

int main(void) {
    /* set handler for SIGINT */
    Signal(SIGINT, parent_sigint_handler);

    /* establish pipe */
    Pipe(fildes);
    if (fcntl(fildes[0], F_SETFL, O_NONBLOCK) == -1) {
        printf("fcntl failed: %s\n", strerror(errno));
        exit(-1);
    }

    /* fork write process */
    if ((write_process = Fork()) == 0) {
        int counter = 1; // counter
        char buffer[BUFFER_SIZE]; // buffer to store message
        /* ignore SIGINT and set handler for SIGUSR1 */
        Signal(SIGINT, SIG_IGN);
        Signal(SIGUSR1, child_sigusr1_handler);
        /* close the unused read end */
        Close(fildes[0]);
        while(signal_flag == 0) {
            /* generate message, increase the counter and then send it to pipe */
            /* finally sleep 1 second */
            snprintf(buffer, BUFFER_SIZE, "I send you %d times.", counter);
            counter++;
            Write(fildes[1], buffer, strlen(buffer));
            sleep(1);
        }
        printf("Child Process 1 is Killed by Parent!\n");
        exit(0);
    }

    /* fork read process */
    if ((read_process = Fork()) == 0) {
        char buffer[BUFFER_SIZE]; // buffer to store message
        /* ignore SIGINT and set handler for SIGUSR1 */
        Signal(SIGINT, SIG_IGN);
        Signal(SIGUSR1, child_sigusr1_handler);
        /* close the unused write end */
        Close(fildes[1]);
        while (signal_flag == 0) {
            /* read message from pipe, and print it */
            // printf("BEFORE\n");
            int read_bytes = Read(fildes[0], buffer, BUFFER_SIZE);
            // printf("END\n");
            /* if read_bytes equals -1 then EAGAIN is set */
            if (read_bytes != -1) {
                buffer[read_bytes] = 0;
                printf("%s\n", buffer);
            }
            /* won't call print if buffer is empty (non-block read end) */
        }
        printf("Child Process 2 is Killed by Parent!\n");
        exit(0);
    }

    /* use pause() to avoid unnecessary spin */
    while(signal_flag == 0) {
        pause();
    }

    /* kill two child processes with sigusr1 */
    Kill(write_process, SIGUSR1);
    Kill(read_process, SIGUSR1);
    /* wait them to be terminated */
    Waitpid(-1, NULL, 0);
    Waitpid(-1, NULL, 0);
    /* close the pipe */
    Close(fildes[0]);
    Close(fildes[1]);
    /* print message and exit */
    printf("Parent Process is Killed!\n");

    return 0;
}
