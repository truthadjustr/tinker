
all: mmap typesize fmo pipeserver pipeclient

myconio.o: myconio.c
	gcc -c myconio.c

pipeserver: pipeserver.cpp myconio.o Makefile
	g++ -o pipeserver pipeserver.cpp myconio.o

pipeclient: pipeclient.cpp myconio.o Makefile
	g++ -o pipeclient pipeclient.cpp myconio.o

fmo: fmo.c Makefile
	g++ -o fmo fmo.cpp

mmap: mmap.c Makefile
	gcc -o mmap mmap.c

typesize: typesize.c Makefile
	gcc -o typesize typesize.c

clean:
	@rm -f *.o *.exe mmap typesize fmo pipeserver pipeclient

.PHOHY: clean
