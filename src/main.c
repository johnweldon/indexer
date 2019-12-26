#include <errno.h>
#include <ftw.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "sqlite3.h"
#include "main.h"
#include "fnv/fnv.h"

static int
callback(void * notUsed, int argc, char ** argv, char ** azColName)
{
    int i;

    for(i = 0; i < argc; i++) {
        fprintf(stdout, "%s = %s\n", azColName[i], argv[i] ? argv[i] : "NULL");
    }

    fprintf(stdout,
            "notUsed %p, argc %d, argv %p, azColName %p\n",
            notUsed, argc, argv, azColName);
    return 0;
}

unsigned long long int
fnv_hash(const char * const fp, const double len)
{
    FILE * fd = 0;

    if ((fd = fopen(fp, "rb")) == NULL) {
        fprintf(stderr, "Can't open file %s; %s\n", fp, strerror(errno));
        fclose(fd);
        return 0;
    };

    const size_t max = len < MAX_LEN ? len : MAX_LEN;

    char * buf = malloc(max);

    size_t read = 0;

    Fnv64_t hash = FNV1A_64_INIT;

    while (0 != (read = fread(buf,  1, max, fd))) {
        fprintf(stdout, "\t\t read %lu bytes\n", read);
        hash = fnv_64a_buf(buf, read, hash);
    }

    fclose(fd);
    free(buf);
    return hash;
}

static int
print_entry(const char * const fp, const struct stat * const info,
            const int typeflag, struct FTW * pathinfo)
{
    double bytes = 0;
    unsigned long long int hash = 0;

    switch (typeflag) {
    case FTW_SL:
    case FTW_SLN:
        fprintf(stdout, " ??? (link) %s\n", fp);
        break;

    case FTW_F:
        bytes = (double)info->st_size;

        if((hash = fnv_hash(fp, bytes)) == 0) {
            hash = 0;
        };

        fprintf(stdout, " ... %s %llx (%0.0f)\n", fp, hash, bytes);

        break;

    case FTW_D:
    case FTW_DP:
        fprintf(stdout, " ... (dir) %s\n", fp);
        break;

    case FTW_DNR:
        fprintf(stdout, " ... (unknown dir) %s\n",  fp);
        break;

    case FTW_NS:
    default:
        fprintf(stdout, " ... (unknown file type: %d) %s\n", typeflag, fp);
        break;
    }

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
    char * zErrMsg = 0;
    int rc = 0;

    if (argc != 3) {
        fprintf(stderr, "Usage: %s <db> <root_dir>\n", argv[0]);
        return(1);
    }

    rc = sqlite3_open(argv[1], &DB);

    if(rc) {
        fprintf(stderr, "Can't open db %s; %s\n", argv[1], sqlite3_errmsg(DB));
        sqlite3_close(DB);
        return(rc);
    }

    rc = sqlite3_exec(DB, INIT_DB, callback, 0, &zErrMsg);

    if(rc) {
        fprintf(stderr, "Can't initialize db %s; %s\n", argv[1], zErrMsg);
        sqlite3_close(DB);
        return(rc);
    }

    rc = print_directory(argv[2]);

    if(rc) {
        fprintf(stderr, "Can't walk dir %s; %s\n", argv[2], strerror(errno));
        sqlite3_close(DB);
        return(rc);
    }

    fprintf(stdout, "Hello indexer!\n");
    sqlite3_close(DB);
    return(0);
}
