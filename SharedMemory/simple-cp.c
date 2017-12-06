// include system headers
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <getopt.h>
#include <errno.h>
#include <string.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/shm.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <time.h>

// include user headers
#include "ring-buffer.h" // ring buffer
#include "semaphore.h" // semaphore

// const definition
#define DEFAULT_BUFFER_CAPACITY 256 // default buffer capacity
#define DEFAULT_BUFFER_NUMBER 8 // default buffer number

#define MAX_INT_ARGUMENT_LENGTH 12 // max integer arument length
#define REFRESH_TIME_INTERVAL 100 // minimal time interval(micro seconds) to update progress bar

// global variables
extern const char  * __progname; // gcc defined as substitute for argv[0]

int verbose_flag = 0; // verbose flag
key_t ipc_key = -1; // IPC key

int buffer_capacity = DEFAULT_BUFFER_CAPACITY; // buffer capacity
int buffer_number = DEFAULT_BUFFER_NUMBER; // buffer number
int semid = -1; // semaphore id
int shmid = -1; // rinb buffer(shared memory) id
char * source_file = NULL; // source file name
off_t source_file_size = 0; // source file size
char * dest_file = NULL; // dest file name
int type = 1; // default type is semaphore implementation

// function list
pid_t Fork(void);

void help(int exit_number) __attribute__((noreturn));
void clean_and_exit(int exit_number) __attribute__((noreturn));

void validation(void);
void initialize(void);
void process(void);

void error_handler(int sig);

void * print_progress(void * argument);

// fork wrapper
pid_t Fork(void) {
    pid_t pid = fork();
    if (pid == -1) {
        printf("fork failed: %s.\n", strerror(errno));
        clean_and_exit(-1);
    }
    return pid;
}

// print help and exit with given number
void help(int exit_number) {
    printf("simple-cp, a simple cp implementation with IPC(Inter-process Communication).\n");
    printf("Usage: simple-cp [options] [SOURCE FILE] [DEST FILE]\n");
    printf("Options:\n");
    printf("-h, --help\tdisplay this help and exit\n");
    printf("-v, --verbose\texplain what is being done\n");
    printf("-t, --type\t1 - semaphore 2 - retry loop other - none\n");
    printf("--buffer-capacity\tspecify buffer capacity (byte)\n");
    printf("--buffer-number\tspecify buffer number\n");
    exit(exit_number);
}

// clean(remove semaphore set/delete ring buffer) and exit with given number
void clean_and_exit(int exit_number) {
    remove_semaphore_set(semid);
    delete_ring_buffer(shmid);
    if (verbose_flag) printf("%s: finished clean process.\n", __progname);
    exit(exit_number);
}

// test if buffer number/buffer capacity/source file is valid
void validation(void) {
    if (buffer_number <= 0 || buffer_capacity <= 0) {
        printf("%s: buffer number/capacity must greater than 0.\n", __progname);
        exit(-1);
    }
    if (access(source_file, R_OK) == -1) {
        printf("%s: cannot access source file %s.\n", __progname, source_file);
        exit(-1);
    }
}

// initialize procedure
void initialize(void) {
    // get the size of source file
    struct stat source_file_stat;
    if (lstat(source_file, &source_file_stat) == -1) {
        printf("lstat failed: %s.\n", strerror(errno));
        if (verbose_flag) printf("%s: failed to get size of input file.\n", __progname);
        exit(-1);
    }
    source_file_size = source_file_stat.st_size;

    // use ftok to create ipc_key used for interprocess communication
    if ((ipc_key = ftok(__progname, 'c')) == -1) {
        printf("ftok failed: %s.\n", strerror(errno));
        if (verbose_flag) printf("%s: failed to generate IPC key.\n", __progname);
        exit(-1);
    }
    if (verbose_flag) printf("%s: IPC key 0x%x generated.\n", __progname, ipc_key);

    // create semaphore set with empty slots set to buffer number
    semid = create_semaphore_set(buffer_number);
    if (verbose_flag) printf("%s: semaphore set created with id 0x%x.\n", __progname, semid);
    // create ring buffer with capacity and number
    shmid = create_ring_buffer();
    if (verbose_flag) printf("%s: ring buffer created with id 0x%x\n", __progname, shmid);
    // set handler for SIGINT(indeed more than SIGINT needed to be handled)
    signal(SIGINT, error_handler);
}

void process(void) {
    // generate child processes' arguments
    char verbose_argument[MAX_INT_ARGUMENT_LENGTH], key_argument[MAX_INT_ARGUMENT_LENGTH];
    char capacity_argument[MAX_INT_ARGUMENT_LENGTH], number_argument[MAX_INT_ARGUMENT_LENGTH];
    char type_argument[MAX_INT_ARGUMENT_LENGTH];
    sprintf(verbose_argument,   "%d",   verbose_flag);
    sprintf(key_argument,       "%d",   ipc_key);
    sprintf(type_argument,      "%d",   type);
    sprintf(capacity_argument,  "%d",   buffer_capacity);
    sprintf(number_argument,    "%d",   buffer_number);

    struct timespec start, end;
    // start timing
    clock_gettime(CLOCK_MONOTONIC, &start);

    // fork two chlid processes and exec put/get processes accordingly
    pid_t putpid, getpid;
    if ((putpid = Fork()) == 0) {
        execl("./simple-cp-put", "simple-cp-put", verbose_argument, key_argument, type_argument, capacity_argument, number_argument, source_file, NULL);
        printf("excel failed: %s.\n", strerror(errno));
        if (verbose_flag) printf("%s: failed to execute put process.\n", __progname);
        exit(-1);
    }
    if (verbose_flag) printf("%s: put process created with process id %d.\n", __progname, putpid);
    if ((getpid = Fork()) == 0) {
        execl("./simple-cp-get", "simple-cp-get", verbose_argument, key_argument, type_argument, capacity_argument, number_argument, dest_file, NULL);
        printf("execl failed: %s.\n", strerror(errno));
        if (verbose_flag) printf("%s: failed to execute get process.\n", __progname);
        exit(-1);
    }
    if (verbose_flag) printf("%s: get process created with process id %d.\n", __progname, getpid);
    
    // create a new thread to print progress bar
    // ONLY shows progress bar in non-verbose mode
    struct RingBuffer * ring_buffer = NULL;
    pthread_t progress_thread;
    if (!verbose_flag) {
        // attach shared ring_buffer
        ring_buffer = attach_ring_buffer(shmid);
        // create thread to print progress bar
        pthread_create(&progress_thread, NULL, print_progress, (void *)ring_buffer); 
    }

    // wait for all child processes
    // if one child process terminated abnormally, then clean and exit
    int status;
    pid_t pid = -1;
    while ((pid = wait(&status))) {
        if (pid == -1) {
            if (errno == ECHILD) {
                if (verbose_flag) printf("%s: all child processes have finished.\n", __progname);
                break; // all child processes have finished
            } else if (errno == EINTR) {
                if (verbose_flag) printf("%s: wait interupted by a signal.\n", __progname);
                continue; // failed due to interupt
            }
        } else if (WIFEXITED(status) && WEXITSTATUS(status) == 0) {
            if (verbose_flag) printf("%s: child process with process id %d has finished.\n", __progname, pid);
            continue; // child process terminated normally
        } else {
            if (verbose_flag) printf("%s: child process with process id %d terminated abnormally.\n", __progname, pid);
            kill(0, SIGINT);
        }
    }

    if (!verbose_flag) {
        // wait for the progress thread to join
        pthread_join(progress_thread, NULL);
        // deattach shared ring_buffer
        deattach_ring_buffer(ring_buffer);
    }

    //end timing
    clock_gettime(CLOCK_MONOTONIC, &end);
    double duration = (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) / 1000000000.0;
    printf("%lu bytes data transferred in %.3f seconds, speed is %.3f MB/s.\n", source_file_size, duration, source_file_size / duration / 1024 / 1024);
}

void error_handler(int sig) {
    // only catch SIGINT so no neet for volatile sig_atomic_t flag
    // clean up
    remove_semaphore_set(semid);
    delete_ring_buffer(shmid);
    // reactive signal to set right return status
    signal(sig, SIG_DFL);
    raise(sig);
}

void * print_progress(void * argument) {
    // acquire shared ring buffer from argument
    struct RingBuffer * ring_buffer = (struct RingBuffer *)argument;

    size_t transferred_size; // transferred file size
    double percent; // complete percentage

    struct winsize window_size; // window size (contains rows and columns)
    int columns; // columns number excluding file name and []
    int dest_file_name_length = strlen(dest_file); // file name length

    while (transferred_size < source_file_size) {
        // acquire transferred bytes from shared ring buffer
        // and calculate complete percentage
        transferred_size = number_of_bytes_transferred(ring_buffer);
        percent = (double)transferred_size / source_file_size;
        
        // use ioctl to acquire console window's size
        if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &window_size) == -1) {
            printf("ioctl failed: %s.\n", strerror(errno));
            if (verbose_flag) printf("%s: failed to get console window's size.\n", __progname);
            exit(-1);
        }
        // calculate columns accordingly
        columns = window_size.ws_col - dest_file_name_length - 4; // 3 = 1 space + 1 [ + 1 ] + 1 preserved

        putchar('\r'); // move to the first of line
        printf("%s :", dest_file); // print file name

        putchar('['); // print [
        for (int count = 0 ; count < columns ; ++count) {
            // print # or space
            char out_char = (count < columns * percent) ? '#' : ' ';
            putchar(out_char);
        }
        putchar(']'); // print ]

        fflush(stdout); // flush stdout explictly
    
        usleep(REFRESH_TIME_INTERVAL); // micro-second sleep
    }
    return NULL;
}       

// program entry
int main(int argc, char * argv[]) {
    // option structs
    static struct option options[] = {
        {"help",            0,  NULL,   'h'},
        {"verbose",         0,  NULL,   'v'},
        {"type",            1,  NULL,   't'},
        {"buffer-capacity", 1,  NULL,   1},
        {"buffer-number",   1,  NULL,   2},
        {0,                 0,  0,      0}
    };
    // parse command line options
    int opt;
    while ((opt = getopt_long(argc, argv, "hvt:", options, NULL)) != -1) {
        switch (opt) {
            case 'v':   verbose_flag = 1;                   break;
            case 't':   type = atoi(optarg);                break;
            case 1:     buffer_capacity = atoi(optarg);     break;
            case 2:     buffer_number = atoi(optarg);       break;
            case 'h':   help(0);                            break;
            case '?':   help(-1);                           break;
        }
    }
    // acquire command line arguments
    if (optind + 1 >= argc) {
        printf("%s: file names expected.\n", argv[0]);
        exit(-1);
    }
    source_file = argv[optind];
    dest_file   = argv[optind + 1];

    // validation
    if (verbose_flag) printf("%s: starting validation process...\n", argv[0]);
    validation();
    if (verbose_flag) printf("%s: validation passed.\n", argv[0]);
    if (verbose_flag) printf("%s: buffer capacity set to %d and buffer number set to %d.\n", argv[0], buffer_capacity, buffer_number);

    // initialize
    if (verbose_flag) printf("%s: starting initialization process...\n", argv[0]);
    initialize();
    if (verbose_flag) printf("%s: initialization completed.\n", argv[0]);

    // copy from source file to dest file
    if (verbose_flag) printf("%s: starting copy process...\n", argv[0]);
    process();
    if (verbose_flag) printf("%s: copy process completed\n", argv[0]);

    // clean up and exit program
    clean_and_exit(0);
}
