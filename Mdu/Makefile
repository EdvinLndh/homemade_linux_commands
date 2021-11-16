 CC=gcc

CCFLAGS = -g -std=gnu11 -Werror -Wall -Wextra -Wpedantic -Wmissing-declarations -Wmissing-prototypes -Wold-style-definition

all: mdu

mdu: mdu.o stack.o
	$(CC) $(CCFLAGS) -o mdu mdu.o stack.o -lpthread
mdu.o: mdu.c stack.h
	$(CC) $(CCFLAGS)  -c mdu.c  
stack.o: stack.c stack.h
	$(CC) $(CCFLAGS) -c stack.c
