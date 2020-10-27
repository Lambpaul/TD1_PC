all: TP1

readline.o: readline.c
	gcc -Wall -g -c readline.c

TP1.o: TP1.c
	gcc -Wall -g -c TP1.c

TP1: readline.o TP1.o
	gcc -o TP1 readline.o TP1.o 

quysh.o: quysh.c
	gcc -Wall -g -c quysh.c

quysh: readline.o quysh.o
	gcc -o quysh readline.o quysh.o

clean:
	rm -f *.o *~ quysh
