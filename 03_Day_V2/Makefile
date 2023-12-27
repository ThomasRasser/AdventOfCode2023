# Variables
# ------------------------------------------------------------
CC = gcc
DEFS = -D_DEFAULT_SOURCE -D_BSD_SOURCE -D_SVID_SOURCE -D_POSIX_C_SOURCE=200809L
CFLAGS = -Wall -g -std=c11 -pedantic $(DEFS) -I/usr/include/x86_64-linux-gnu
LDFLAGS = -lcurl -lm -lrt
OBJECTS = main.o

# Targets
# ------------------------------------------------------------
.PHONY: all clean
all: main

# Linking
# ------------------------------------------------------------
main: $(OBJECTS)
				$(CC) -o $@ $^ $(LDFLAGS)

# Compiling
# ------------------------------------------------------------
%.o: %.c
				$(CC) $(CFLAGS) -c -o $@ $<

# Cleaning
# ------------------------------------------------------------
clean:
				rm -rf *.o main