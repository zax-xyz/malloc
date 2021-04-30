CC=gcc
CFLAGS=-fsanitize=address -Wall -Werror -std=gnu11 -g -lm
LDFLAGS=-lasan

SRCDIR=src
INCDIR=include
BUILDDIR=build

INCLUDES=-I$(INCDIR)

.PHONY: tests debug clean

debug: DEBUG=-DDEBUG
debug: tests

tests: $(BUILDDIR)/tests.o $(BUILDDIR)/virtual_alloc.o $(BUILDDIR)/helpers.o
	$(CC) $(CFLAGS) $(LDFLAGS) $^ -o $@

$(BUILDDIR):
	mkdir -p $(BUILDDIR)

$(BUILDDIR)/tests.o: tests.c | $(BUILDDIR)
	$(CC) $(CFLAGS) $(INCLUDES) $(DEBUG) -c -o $@ $<

$(BUILDDIR)/%.o: $(SRCDIR)/%.c $(INCDIR)/%.h | $(BUILDDIR)
	$(CC) $(CFLAGS) $(INCLUDES) $(DEBUG) -c -o $@ $<

clean:
	rm -f tests
	rm -f $(BUILDDIR)/*.o
	rmdir $(BUILDDIR)