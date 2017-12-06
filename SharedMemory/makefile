CC = gcc
CFLAGS = -Wall -Werror
LDLIBS = -lpthread

HEADERS = $(shell find ./ -name "*.h")
SOURCES = $(shell find ./ -name "*.c")
OBJECTS = $(patsubst %.c, %.o, $(SOURCES))
TARGET = simple-cp simple-cp-put simple-cp-get

all: $(TARGET)

$(TARGET): %: %.o semaphore.o ring-buffer.o
		$(CC) $< semaphore.o ring-buffer.o $(LDLIBS) -o $@

%.o: %.c semaphore.c ring_buffer.c $(HEADERS)
		$(CC) $(CFLAGS) -c $< semaphore.c ring-buffer.c -o $@

.PHONY: clean

clean:
		rm -f $(OBJECTS)
		rm -f $(TARGET)
