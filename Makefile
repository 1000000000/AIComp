USERNAME = $(shell head -1 credentials)
DEVKEY = $(shell head -2 credentials | tail -1)
DEFS = -D "USERNAME=$(USERNAME)" -D "DEVKEY=$(DEVKEY)"
LDFLAGS = -lcurl -ljson-c
CFLAGS = -Wall
BAK_SHELL := $(SHELL)
SHELL := /bin/bash
BINARIES := $(basename $(shell comm -12 <(ls -1 *.c) <(ls -1 -I main.c -I game_state.c)))
SHELL := $(BAK_SHELL)

.PHONY: all
all: $(BINARIES)

game_state.o: game_state.c game_state.h
	$(CC) $(CFLAGS) -o game_state.o -c game_state.c

main.o: main.c ai.h
	$(CC) $(CFLAGS) $(DEFS) -o main.o -c main.c

%: %.c ai.h main.o game_state.h game_state.o
	$(CC) $(CFLAGS) -o $@ main.o game_state.o $< $(LDFLAGS)

.PHONY: debug
debug: CFLAGS += -g -O0
#debug: DEFS += -DDEBUG
debug: all

.PHONY: clean
clean:
	rm -f $(BINARIES) main.o game_state.o
