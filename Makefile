CC=gcc
CFLAGS=-Wall -Werror -std=gnu11 -g -lm

SRCDIR=src
INCDIR=include
BUILDDIR=build

INCLUDES=-I$(INCDIR)

.PHONY: tests debug clean

debug: DEBUG=-DDEBUG
debug: tests

tests: DEBUG=-DDEBUG
tests: $(BUILDDIR)/tests.o $(BUILDDIR)/virtual_alloc.o $(BUILDDIR)/helpers.o
	$(CC) $^ -o $@

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