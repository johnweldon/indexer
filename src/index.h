#ifndef _SRC_INDEX_H_
#define _SRC_INDEX_H_

#include <unistd.h>

#include "sqlite/sqlite3.h"

extern sqlite3 * DB;

extern char * db_name;
extern char * sql_file;
extern char * root_dir;

extern const size_t MAX_PATH;
extern const size_t MAX_LEN;

extern const char * const INIT_DB;
extern const char * const ADD_FILE;
extern const char * const ADD_BLOB;
extern const char * const ADD_FILE_BLOB;
extern const char * const ADD_FILE_TAG;

int process_directory(const char * const);
int db_result_handler(void *, int, char **, char **);

#endif /*_SRC_INDEX_H_*/
