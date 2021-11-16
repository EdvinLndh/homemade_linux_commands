#mexec: 
CC=gcc

CCFLAGS = -g -std=gnu11 -Werror -Wall -Wextra -Wpedantic -Wmissing-declarations -Wmissing-prototypes -Wold-style-definition

all: mexec

mexec: mexec.o
	$(CC) $(CCFLAGS) -o mexec mexec.o
mexec.o: mexec.c
	$(CC) $(CCFLAGS) -c -o mexec.o mexec.c
