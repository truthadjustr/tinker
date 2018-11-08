
all: mmap typesize fmo

fmo: fmo.c Makefile
	g++ -o fmo fmo.cpp

mmap: mmap.c Makefile
	gcc -o mmap mmap.c

typesize: typesize.c Makefile
	gcc -o typesize typesize.c

clean:
	@rm -f *.o *.exe mmap typesize fmo

.PHOHY: clean
