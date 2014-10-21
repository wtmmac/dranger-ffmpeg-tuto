LIBS = -lz -lm -lavutil -lavcodec -lavformat -lswscale `sdl-config --cflags --libs`
CC = gcc
CFLAGS = -g -Wall

TARGETS = $(patsubst %.c, %, $(wildcard *.c))
SOURCES = $(wildcard *.c)

all: $(TARGETS)

%: %.c
		$(CC) $< $(CFLAGS) $(LIBS) -o $@


clean:
		-rm -f $(TARGETS)
