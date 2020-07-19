CC          = gcc
CFLAGS      = -g -Wall -O2
LDFLAGS     = -lz
prefix      = /usr/local
exec_prefix = $(prefix)/bin

src = $(wildcard *.c)
obj = $(src:.c=.o)

dude: $(obj)
	$(CC) -o $@ $^ $(LDFLAGS)

.PHONY: clean
clean:
	rm -f $(obj) dude

.PHONY: install
install:
	cp dude $(exec_prefix)







