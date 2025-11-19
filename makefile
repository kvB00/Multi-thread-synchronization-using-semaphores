CC = g++
CFLAGS = -g -Wall -pthread -lm

all: cse4001_sync

cse4001_sync: cse4001_sync.cpp semaphore_class.h
	$(CC) $(CFLAGS) cse4001_sync.cpp -o cse4001_sync

clean:
	rm -f cse4001_sync main


	

