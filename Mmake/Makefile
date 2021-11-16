CC=gcc

CCFLAGS = -g -std=gnu11 -Werror -Wall -Wextra -Wpedantic -Wmissing-declarations -Wmissing-prototypes -Wold-style-definition

#all: mmake

mmake: mmake.o parser.o
	$(CC) -o mmake mmake.o parser.o

mmake.o: mmake.c parser.h
	$(CC) $(CCFLAGS) -c mmake.c 

parser.o: parser.c parser.h
	$(CC) $(CCFLAGS) -c parser.c
