CFLAGS=-g -O2 -Wall -Wextra -Werror -Isrc -rdynamic -Wno-missing-field-initializers -DNDEBUG $(OPTFLAGS)
LIBS=-ldl $(OPTLIBS)
PREFIX?=/usr/local

HEADERS=$(wildcard src/**/*.h)

SOURCES=$(wildcard src/**/*.c src/*.c)
OBJECTS=$(patsubst %.c,%.o,$(SOURCES))

TEST_SRC=$(wildcard tests/*_tests.c)
TESTS=$(patsubst tests/%.c,%,$(TEST_SRC))

PROGRAMS_SRC=$(wildcard src/bin/*.c)
PROGRAMS=$(patsubst src/bin/%.c,bin/%,$(PROGRAMS_SRC))

TARGET=build/libdumhet.a
SO_TARGET=$(patsubst %.a,%.so,$(TARGET))

# The Target Build
all: $(TARGET) $(SO_TARGET) $(PROGRAMS) tests 

dev: CFLAGS=-g -Wall -Isrc -Wall -Wextra -Werror -Wno-missing-field-initializers $(OPTFLAGS)
dev: all

$(TARGET): CFLAGS += -fPIC
$(TARGET): build $(OBJECTS)
	ar rcs $@ $(OBJECTS)
	ranlib $@

$(SO_TARGET): $(TARGET) $(OBJECTS)
	$(CC) -shared -o $@ $(OBJECTS)

build:
	@mkdir -p build
	@mkdir -p bin/tests

# The Unit Tests
.PHONY: tests
tests: $(TESTS)
	sh ./runtests.sh

$(OBJECTS): %: $(HEADERS)

$(TESTS): %: $(TARGET) tests/%.c
	$(CC) $(LIBS) $(CFLAGS) tests/$@.c $< -o bin/tests/$@

$(PROGRAMS): %: $(TARGET) src/%.c
	$(CC) $(LIBS) $(CFLAGS) src/$@.c $< -o $@

valgrind:
	VALGRIND="valgrind --leak-check=full --error-exitcode=1" $(MAKE)

TAGS: $(SOURCES) $(PROGRAMS_SRC) $(TEST_SRC) $(HEADERS)
	etags $^

# The Cleaner
clean:
	rm -rf build bin $(OBJECTS) $(PROGRAMS)
	rm -f TAGS tests.log

# The Install
install: all
	install -d $(DESTDIR)/$(PREFIX)/lib/
	install $(TARGET) $(DESTDIR)/$(PREFIX)/lib/

# The Checker
BADFUNCS='[^_.>a-zA-Z0-9](str(n?cpy|n?cat|xfrm|n?dup|str|pbrk|tok|_)|stpn?cpy|a?sn?printf|byte_)' 
check:
	@echo Files with potentially dangerous functions.
	@egrep $(BADFUNCS) $(HEADERS) $(SOURCES) $(PROGRAMS_SRC) || true
