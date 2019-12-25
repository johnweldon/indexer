CFLAGS = -Oz -O3
INCLUDE = \
		-I ./src \
		-I ./src/sqlite \
		-I ./src/fnv \
		-I /usr/local/include \
		-I /usr/local/opt/icu4c/include
LIBDIR = \
		-L /usr/local/opt/icu4c/lib
LIBS = \
		-lreadline \
		-lncurses \
		-licui18n \
		-licudata \
		-licuio \
		-licutu \
		-licuuc \
		-ldl

CC = clang $(CFLAGS)

SQLITE_FEATURES = \
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
	$< test.db src

.PHONY: clean
clean:
	-rm -rf bin obj

bin:
	mkdir bin

obj:
	mkdir obj

obj/sqlite3.o: src/sqlite/sqlite3.c | obj
	$(CC) $(CFLAGS) $(SQLITE_FEATURES) $(INCLUDE) -o $@ $^ -c

obj/shell.o: src/sqlite/shell.c | obj
	$(CC) $(CFLAGS) $(SQLITE_FEATURES) $(INCLUDE) -o $@ $^ -c

obj/main.o: src/main.c | obj
	$(CC) $(CFLAGS) $(INCLUDE) -o $@ $^ -c

bin/sqlite3: obj/sqlite3.o obj/shell.o | bin
	$(CC) $(CFLAGS) $(SQLITE_FEATURES) $(LIBDIR) -o $@ $^ $(LIBS)

bin/ix: obj/sqlite3.o obj/main.o | bin
	$(CC) $(CFLAGS) $(SQLITE_FEATURES) $(LIBDIR) -o $@ $^ $(LIBS)
