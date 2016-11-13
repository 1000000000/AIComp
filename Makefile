USERNAME = $(shell head -1 credentials)
DEVKEY = $(shell head -2 credentials | tail -1)
DEFS = -D "USERNAME=$(USERNAME)" -D "DEVKEY=$(DEVKEY)"
LDFLAGS = -lcurl -ljson-c
CFLAGS = -Wall
BAK_SHELL := $(SHELL)
SHELL := /bin/bash
BINARIES := $(basename $(shell comm -12 <(ls -1 *.c) <(ls -1 -I main.c)))
SHELL := $(BAK_SHELL)

.PHONY: all
all: $(BINARIES)

main.o: main.c ai.h
	$(CC) $(CFLAGS) $(DEFS) -o main.o -c main.c

%: %.c main.o ai.h
	$(CC) $(CFLAGS) -o $@ main.o $< $(LDFLAGS)

.PHONY: debug
debug: CFLAGS += -g -O0
debug: aicomp

.PHONY: clean
clean:
	rm -f $(BINARIES) main.o
