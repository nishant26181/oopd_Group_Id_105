# Root Makefile - delegates build and clean targets to src/
all:
	$(MAKE) -C src

clean:
	$(MAKE) -C src clean
