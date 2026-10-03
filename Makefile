CC ?= cc
AR ?= ar
CFLAGS ?= -std=gnu11 -O2 -g -Wall -Wextra -Wno-unused-parameter -Wno-unused-variable -Wno-deprecated-declarations -DGL_SILENCE_DEPRECATION
CPPFLAGS += $(shell pkg-config --cflags sdl2 libjpeg)

.PHONY: all clean

all: libwtk-ng.a

libwtk-ng.a: wtk_ng.o
	$(AR) rcs $@ $^

wtk_ng.o: wtk_ng.c wt.h
	$(CC) $(CPPFLAGS) $(CFLAGS) -I. -c $< -o $@

clean:
	rm -f wtk_ng.o libwtk-ng.a
