SHELL := bash
.ONESHELL:
.SHELLFLAGS := -eu -o pipefail -c
.DELETE_ON_ERROR:
MAKEFLAGS += --warn-undefined-variables
MAKEFLAGS += --no-builtin-rules

CFLAGS += -Os -O2
INCLUDE += \
		-I ./src \
		-I /usr/local/include \
		-I /usr/local/opt/icu4c/include
LIBDIR += \
		-L ./src/fnv \
		-L /usr/local/opt/icu4c/lib
LIBS += \
		-lreadline \
		-lncurses \
		-licui18n \
		-licudata \
		-licuio \
		-licutu \
		-licuuc \
		-ldl -lm -lpthread

SQLITE_FEATURES += \
		-DSQLITE_THREADSAFE=1 \
		-DSQLITE_ENABLE_FTS4 \
		-DSQLITE_ENABLE_FTS5 \
		-DSQLITE_ENABLE_JSON1 \
		-DSQLITE_ENABLE_RTREE \
		-DSQLITE_ENABLE_COLUMN_METADATA \
		-DSQLITE_ENABLE_ICU \
		-DSQLITE_ENABLE_GEOPOLY \
		-DSQLITE_ENABLE_EXPLAIN_COMMENTS \
		-DSQLITE_HAVE_USLEEP -DSQLITE_HAVE_READLINE


.PHONY: all
all: bin/ix bin/sqlite3

.PHONY: run
run: bin/ix
	$< -d test.db -r src

.PHONY: clean
clean:
	-rm -rf bin obj
	-(cd src/fnv; make clean)

.PHONY: clobber
clobber: clean
	-rm -rf test.db
	-(cd src/fnv; make clobber)

bin:
	mkdir bin

obj:
	mkdir obj

obj/sqlite3.o: src/sqlite/sqlite3.c | obj
	$(CC) $(CFLAGS) $(SQLITE_FEATURES) $(INCLUDE) -o $@ $^ -c

obj/shell.o: src/sqlite/shell.c | obj
	$(CC) $(CFLAGS) $(SQLITE_FEATURES) $(INCLUDE) -o $@ $^ -c

obj/%.o: src/%.c | obj src/fnv/longlong.h
	$(CC) $(CFLAGS) $(INCLUDE) -o $@ $^ -c

src/fnv/longlong.h:
	(cd src/fnv; make longlong.h)

src/fnv/libfnv.a: src/fnv/*.c src/fnv/longlong.h
	(cd src/fnv; make libfnv.a)

bin/sqlite3: obj/sqlite3.o obj/shell.o | bin
	$(CC) $(CFLAGS) $(SQLITE_FEATURES) $(LIBDIR) -o $@ $^ $(LIBS)

bin/ix: obj/sqlite3.o obj/main.o obj/index.o src/fnv/libfnv.a | bin
	$(CC) $(CFLAGS) $(SQLITE_FEATURES) $(LIBDIR) -o $@ $^ $(LIBS) -lfnv
