CC = arm-linux-gnueabihf-gcc
CFLAGS = -static -O2 -Wall -Wextra

all: test_seek_arm

test_seek_arm: test_seek.c
	$(CC) $(CFLAGS) $< -o $@

clean:
	rm -f test_seek_arm

.PHONY: all clean
