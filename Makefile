all: readline TP1

readline.o: readline.c
	gcc -Wall -g -c readline.c

readline: readline.o
	gcc -o readline readline.o

TP1.o: TP1.c
	gcc -Wall -g -c TP1.c

TP1: TP1.o
	gcc -o TP1 TP1.o

clean:
	rm -f *.o *~ 
