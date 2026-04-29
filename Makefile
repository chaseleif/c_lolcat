CC:=gcc
#CFLAGS:=-lm -O0 -ggdb3
CFLAGS:=-lm -O3
BIN=lolcat

all: $(BIN)

$(BIN): lol.c
	$(CC) $(CFLAGS) $^ -o $@
