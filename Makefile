CC = clang -Oz -O3 -I ./src -I ./src/sqlite -I /usr/local/include -I /usr/local/opt/icu4c/include -L /usr/local/opt/icu4c/lib
SQLITE_SOURCES = src/sqlite/sqlite3.c src/sqlite/shell.c
SQLITE_FEATURES = -DSQLITE_THREADSAFE=1 \
		-DSQLITE_ENABLE_FTS4 \
		-DSQLITE_ENABLE_FTS5 \
		-DSQLITE_ENABLE_JSON1 \
		-DSQLITE_ENABLE_RTREE \
		-DSQLITE_ENABLE_COLUMN_METADATA \
		-DSQLITE_ENABLE_ICU \
		-DSQLITE_ENABLE_GEOPOLY \
		-DSQLITE_ENABLE_EXPLAIN_COMMENTS \
		-DSQLITE_HAVE_USLEEP -DSQLITE_HAVE_READLINE
IX_SOURCES = src/main.c src/sqlite/sqlite3.c


.PHONY: all
all: bin/ix

.PHONY: run
run: bin/ix
	$< test.db src

.PHONY: clean
clean:
	-rm -rf bin

bin:
	mkdir bin

bin/sqlite3: bin $(SQLITE_SOURCES)
	$(CC) -o $@ $(SQLITE_FEATURES) $(SQLITE_SOURCES) -ldl -lreadline -lncurses -licui18n -licudata -licuio -licutu -licuuc

bin/ix: bin $(IX_SOURCES)
	$(CC) -o $@ $(SQLITE_FEATURES) $(IX_SOURCES) -ldl -lreadline -lncurses -licui18n -licudata -licuio -licutu -licuuc
