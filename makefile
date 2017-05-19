CC = gcc
CFLAGS =-lm -I.
DEPS = Controller.h

%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

Disco: main.o Controller.o
	gcc -o Disco main.o Controller.o -lm -I.

	
