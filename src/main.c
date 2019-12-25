#include <stdlib.h>
#include <stdio.h>

#include "sqlite3.h"

int
main(int argc, char ** argv)
{
    sqlite3 * db;
    char * zErrMsg = 0;
    int rc;

    if (argc != 3) {
        fprintf(stderr, "Usage: %s <db> <root_dir>\n", argv[0]);
        return(1);
    }

    rc = sqlite3_open(argv[1], &db);

    if(rc) {
        fprintf(stderr, "Can't open db %s; %s\n", argv[1], sqlite3_errmsg(db));
        sqlite3_close(db);
        return(rc);
    }

    fprintf(stdout, "Hello indexer!\n");
    sqlite3_close(db);
    return(0);
}
