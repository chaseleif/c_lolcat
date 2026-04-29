CC:=gcc
#CFLAGS:=-lm -O0 -ggdb3
CFLAGS:=-lm -O3 --fast-math
BIN=lolcat

all: $(BIN)

$(BIN): lol.c
	$(CC) $(CFLAGS) $^ -o $@

.PHONY: clean
clean:
	rm -f $(BIN)
