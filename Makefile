CC = gcc
CFLAGS = -Wall -ggdb
LDFLAGS = 
BIN = irony

all:	$(BIN)

irony:	irony.o uinput.o rc5.o sirc.o recs80.o sharp.o config.o

clean:
	rm -f $(BIN) *~ core *.o

poke:	irony
	./irony

.PHONY:	clean
