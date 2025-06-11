CC?=gcc
CFLAGS?=-Wall -Wextra -O2 -g -Iinclude

ifdef ERMFS_LOCKLESS
CFLAGS+=-DERMFS_LOCKLESS
endif
LDFLAGS?=-lz -lpthread

SRCS=src/erm_alloc.c src/ermfs.c src/erm_compress.c src/ermfd.c src/ermfs_lockless.c
OBJS=$(SRCS:.c=.o)
LIB=libermfs.a

all: $(LIB)

$(LIB): $(OBJS)
	$(AR) rcs $@ $^

clean:
	rm -f $(OBJS) $(LIB)

.PHONY: all clean
