CC = gcc
CFLAGS = -std=c99 -Iinclude -pthread
MAIN_SRC = src/strutil.c main.c
MAIN_TARGET = strutil_demo

.PHONY: all clean test

all: $(MAIN_TARGET)

$(MAIN_TARGET): $(MAIN_SRC)
	$(CC) $(CFLAGS) $(MAIN_SRC) -o $(MAIN_TARGET)

test:
	$(MAKE) -C test

clean:
	rm -f $(MAIN_TARGET)
	$(MAKE) -C test clean
