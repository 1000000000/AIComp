USERNAME = $$(head -1 credentials)
DEVKEY = $$(head -2 credentials | tail -1)
CFLAGS = -Wall -D "USERNAME=$(USERNAME)" -D "DEVKEY=$(DEVKEY)"
LDFLAGS = -lcurl -ljson-c
objects = main.o

.PHONY: all
all: aicomp

aicomp: $(objects)
	$(CC) $(CFLAGS) -o aicomp $(objects) $(LDFLAGS)

main.o: main.c
	$(CC) $(CFLAGS) -c main.c

.PHONY: debug
debug: CFLAGS += -g -O0
debug: aicomp

.PHONY: clean
clean:
	rm -f aicomp $(objects)
