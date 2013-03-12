WFLAGS=-Wall -Wextra -Werror -Wno-missing-field-initializers
CFLAGS=-g -O2 -Isrc -rdynamic -DNDEBUG $(WFLAGS) $(OPTFLAGS)
LIBS=-ldl $(OPTLIBS)
PREFIX?=/usr/local

DHTHEADERS=$(wildcard src/dht/*.h)
LCTHWHEADERS=$(wilcard src/lcthw/*.h)

SOURCES=$(wildcard src/**/*.c src/*.c)
OBJECTS=$(patsubst src/%.c,build/%.o,$(SOURCES))

TEST_SRC=$(wildcard tests/*_tests.c)
TESTS=$(patsubst tests/%.c,bin/tests/%,$(TEST_SRC))

PROGRAMS_SRC=$(wildcard src/bin/*.c)
PROGRAMS=$(patsubst src/bin/%.c,bin/%,$(PROGRAMS_SRC))

TARGET=build/libdumhet.a
SO_TARGET=$(patsubst %.a,%.so,$(TARGET))

all: $(TARGET) $(SO_TARGET) $(PROGRAMS) tests

dev: CFLAGS=-g -Isrc $(WFLAGS) $(OPTFLAGS)
dev: all

$(TARGET): CFLAGS += -fPIC
$(TARGET): $(OBJECTS)
	ar rcs $@ $(OBJECTS)
	ranlib $@

$(SO_TARGET): $(TARGET) $(OBJECTS)
	$(CC) -shared -o $@ $(OBJECTS)

.PHONY: tests
tests: $(TESTS)
	sh ./runtests.sh

build/lcthw/%.o: src/lcthw/%.c $(LCTHWHEADERS)
	@mkdir -p build/lcthw
	$(CC) $(CFLAGS) -c -o $@ $<

build/%.o: src/%.c $(LCTHWHEADERS) $(DHTHEADERS)
	@mkdir -p build
	$(CC) $(CFLAGS) -c -o $@ $<

bin/tests/%: $(TARGET) tests/%.c tests/minunit.h
	@mkdir -p bin/tests
	$(CC) $(LIBS) $(CFLAGS) tests/$*.c $< -o $@

$(PROGRAMS): %: $(TARGET) src/%.c
	@mkdir -p bin
	$(CC) $(LIBS) $(CFLAGS) src/$@.c $< -o $@

valgrind:
	VALGRIND="valgrind --leak-check=full --error-exitcode=1" $(MAKE)

TAGS: $(SOURCES) $(TEST_SRC) $(DHTHEADERS) $(LCTHWHEADERS)
	etags $^

clean:
	rm -rf build bin TAGS tests.log

# The Install
install: all
	install -d $(DESTDIR)/$(PREFIX)/lib/
	install $(TARGET) $(DESTDIR)/$(PREFIX)/lib/

# The Checker
BADFUNCS='[^_.>a-zA-Z0-9](str(n?cpy|n?cat|xfrm|n?dup|str|pbrk|tok|_)|stpn?cpy|a?sn?printf|byte_)' 
check:
	@echo Files with potentially dangerous functions.
	@egrep $(BADFUNCS) $(HEADERS) $(SOURCES) $(PROGRAMS_SRC) || true
