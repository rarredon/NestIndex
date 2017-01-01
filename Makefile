CC=gcc
CFLAGS=-lm -Wall
SOURCE=NestIndex.c
EXECUTABLE=NestIndex

all: 
	$(CC) $(CFLAGS) $(SOURCE) -o $(EXECUTABLE)
