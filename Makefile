CC?=gcc
CFLAGS?=-Wall -Wextra -O2 -g -Iinclude

SRCS=src/erm_alloc.c src/ermfs.c
OBJS=$(SRCS:.c=.o)
LIB=libermfs.a

all: $(LIB)

$(LIB): $(OBJS)
	$(AR) rcs $@ $^

clean:
	rm -f $(OBJS) $(LIB)

.PHONY: all clean
