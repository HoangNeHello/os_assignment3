CC = gcc
CFLAGS = -Wall -Wpointer-arith -Wstrict-prototypes -std=gnu89 -fPIC -MMD -MP -lpthread

all: test-mergesort test-parallel-mergesort

# This builds the original executable 
test-mergesort: test-mergesort.o mergesort.o
	$(CC) $(CFLAGS) -o $@ $?

# This builds the custom test 
test-parallel-mergesort: test-parallel-mergesort.c mergesort.c
	$(CC) -Wall -fPIC -lpthread -o $@ $^

.PHONY: clean
clean:
	/bin/rm -f *.o *.d test-mergesort test-parallel-mergesort