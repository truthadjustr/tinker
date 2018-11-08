
mmap: mmap.c Makefile
	gcc -o mmap mmap.c

clean:
	@rm -f *.o mmap

.PHOHY: clean
