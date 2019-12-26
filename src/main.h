#ifndef _SRC_MAIN_
#define _SRC_MAIN_

#include <stdlib.h>

#include "sqlite3.h"

sqlite3 * DB = 0;

static const size_t MAX_LEN = 1 << 30;
static const char * const INIT_DB =
    "CREATE TABLE IF NOT EXISTS files ("
    "  hash TEXT,"
    "  path TEXT,"
    "  size INTEGER)";

#endif /* _SRC_MAIN_ */
