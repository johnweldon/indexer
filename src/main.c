#include <errno.h>
#include <ftw.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "sqlite3.h"

static int
callback(void * notUsed, int argc, char ** argv, char ** azColName)
{
    int i;

    for(i = 0; i < argc; i++) {
        fprintf(stdout, "%s = %s\n", azColName[i], argv[i] ? argv[i] : "NULL");
    }

    fprintf(stdout, "\n");
    return 0;
}

static int
print_entry(const char * const fp, const struct stat * const info,
            const int typeflag, struct FTW * pathinfo)
{
    const double bytes = (double)info->st_size;
    fprintf(stdout, " ... %s (%0.0f)\n", fp, bytes);
    return 0;
}

static int
print_directory(const char * const dir)
{
    int result;

    if (NULL == dir || '\0' == *dir) {
        return errno  = EINVAL;
    }

    result = nftw(dir, print_entry, 15, FTW_PHYS) ;

    if (result >= 0) {
        errno = result;
    }

    return errno;
}

int
main(int argc, char ** argv)
{
    sqlite3 * db;
    char * zErrMsg = 0;
    int rc;
    const char * init_db =
        "CREATE TABLE IF NOT EXISTS files "
        "(hash TEXT, path TEXT, size INTEGER)";

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

    rc = sqlite3_exec(db, init_db, callback, 0, &zErrMsg);

    if(rc) {
        fprintf(stderr, "Can't initialize db %s; %s\n", argv[1], zErrMsg);
        sqlite3_close(db);
        return(rc);
    }

    rc = print_directory(argv[2]);

    if(rc) {
        fprintf(stderr, "Can't walk dir %s; %s\n", argv[2], strerror(errno));
        sqlite3_close(db);
        return(rc);
    }

    fprintf(stdout, "Hello indexer!\n");
    sqlite3_close(db);
    return(0);
}
