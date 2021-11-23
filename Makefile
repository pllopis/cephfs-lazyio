BINS=lazyio.so
CC=g++

all: $(BINS)

lazyio.so: lazyio.c
	$(CC) $(CFLAGS) -shared -fPIC $< -o $@  -ldl

clean:
	rm -f $(BINS) *.o
