CC=gcc
CFLAGS=-fsanitize=address -Wall -Werror -std=gnu11 -g -lm
LDFLAGS=-lasan

SRCDIR=src
INCDIR=include
LIBDIR=lib
BUILDDIR=build

INCLUDES=-I$(INCDIR)
TESTINCLUDES=-I$(LIBDIR)
TESTLIBS=-Llib -lcmocka-static

DEPS=$(BUILDDIR)/virtual_alloc.o $(BUILDDIR)/helpers.o

.PHONY: tests debug clean

tests: $(BUILDDIR)/tests.o $(DEPS)
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS) $(TESTLIBS)

debug: DEBUG=-DDEBUG
debug: tests

run_tests: tests
	./tests

$(BUILDDIR):
	mkdir -p $(BUILDDIR)

$(BUILDDIR)/tests.o: tests.c | $(BUILDDIR)
	$(CC) $(CFLAGS) $(INCLUDES) $(TESTINCLUDES) $(DEBUG) -c -o $@ $<

$(BUILDDIR)/%.o: $(SRCDIR)/%.c $(INCDIR)/%.h | $(BUILDDIR)
	$(CC) $(CFLAGS) $(INCLUDES) $(DEBUG) -c -o $@ $<

clean:
	rm -f tests
	rm -f $(BUILDDIR)/*.o
	rmdir $(BUILDDIR)