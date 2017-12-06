# OperatingSystemExperiments

## Introduction

This is a set of 4 experiments about Operating System, in School of Computer Science and Technology, Huazhong University of Science and Technology.

1. Process Control, connect 2 sub-processes with pipe, 1 write to buffer, 1 read from buffer.
2. Thread Communication, create 2 threads, 1 calculate 1 + 2 + 3 + ... + 100, 1 print result after each add operation.
3. Shared Memory, implement a fake-cp using shared memory / ring buffer, producer-consumer problem.
4. FileDirectory, implement a program similar with ls -lR.

The author is an undergraduate. Issues/Pull Requests are welcomed.
I'm glad if you're with similar experiment and what I "doodled" can inspire you. Though I have NO TOLERANCE OF cheating and copying my code directly in your experiment.

## Tips

1. experiment 1 is mainly about system calls such as fork/read/write/pipe/etc.
2. experiment 2 focuses on POSIX Threads and System V IPC's semaphores. You should make sure print result JUST between two add operations.
3. experiment 3 focuses on System V IPC's semaphores and shared memory. It's an interesting problem considering this ring buffer is a queue with only 1 producer and 1 consumer.
4. experiment 4 requires you to know basis about Unix file system - files as well as directory tree structures. System calls like stat/lstat/etc. are required.

## Reference

* Use `man` more often
* Advanced Programming in the Unix Environment, W.Richard Stevens
* Unix Network Programming, Volume 2: Interprocess Communications, W.Richard Stevens
* Maybe Stackoverflow, etc.

## Features

Experiment 3 is more interesting to me and I spent most of my time on it.

Queue is a special data structure, only 1 producer and 1 consumer makes it more special.

General Mutex/Semaphores works well, but mutex lock is unnecessary.
What's more, there exists the implementation of lock-free queues - Using retry loop instead.

Makefiles are provided to all experiments.
Try `./simple-cp -h` to see what I've done with experiment 3.

## License
MIT License (c) Cosidian/码龙黑曜
