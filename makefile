CC=gcc
CXX=g++
CFLAGS+=-O3 -Wall
LFLAGS+=-lm

siddump: siddump.o cpu.o
	$(CC) $(CFLAGS) $(LFLAGS) -o $@ $^
	strip $@
