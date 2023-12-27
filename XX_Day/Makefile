# Variables
# ------------------------------------------------------------
CC = gcc
DEFS = -D_DEFAULT_SOURCE -D_BSD_SOURCE -D_SVID_SOURCE -D_POSIX_C_SOURCE=200809L
CFLAGS = -Wall -g -std=c99 -pedantic $(DEFS)
OBJECTS = main.o

# Targets
# ------------------------------------------------------------
.PHONY: all clean
all: main

# Linking
# ------------------------------------------------------------
main: $(OBJECTS)
				$(CC) $(LDFLAGS) -o $@ $^

# Compiling
# ------------------------------------------------------------
%.o: %.c
				$(CC) $(CFLAGS) -c -o $@ $<

# Cleaning
# ------------------------------------------------------------
clean:
				rm -rf *.o main