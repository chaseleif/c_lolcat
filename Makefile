CC:=gcc
#CFLAGS:=-O0 -ggdb3 -lm
CFLAGS:=-march=native -O3 --fast-math
LDFLAGS:=-lm
BIN=lolcat

all: $(BIN)

$(BIN): lol.c
	$(CC) -o $@ $^ $(CFLAGS) $(LDFLAGS)

.PHONY: clean
clean:
	rm -f $(BIN)
