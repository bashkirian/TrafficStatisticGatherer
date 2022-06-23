CC        = gcc
CC_FLAGS  = -g -Wall
default : all

all: PrintStat

client: PrintStat.c
	$(CC) $(CFLAGS) -o PrintStat PrintStat.c -lrt

clean:
	rm -f PrintStat \
        rm -f PrintStat.o

