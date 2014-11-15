LIBS = -lz -lm -lavutil -lavcodec -lavformat -lswscale `sdl-config --cflags --libs`

ifeq ($(shell uname), Darwin) 
    LIBS += -lbz2
    $(warning  $(LIBS))
endif

CC = gcc

CFLAGS = -std=c99 -g -Wall

#ifeq ($(shell uname), Darwin) 
#    CFLAGS += -I/usr/local/ffmpeg-0.11.1/include/ -L/usr/local/ffmpeg-0.11.1/lib/
#    $(warning  $(CFLAGS))
#endif

TARGETS = $(patsubst %.c, %, $(wildcard *.c))
SOURCES = $(wildcard *.c)

all: $(TARGETS)

%: %.c
		$(CC) $< $(CFLAGS) $(LIBS) -o $@


clean:
		-rm -f $(TARGETS)
