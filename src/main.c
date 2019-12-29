#include <errno.h>
#include <ftw.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "sqlite3.h"
#include "main.h"
#include "fnv/fnv.h"

sqlite3 * DB = 0;

static const size_t MAX_LEN = 1 << 30;
static const char * const INIT_DB =
    "CREATE TABLE IF NOT EXISTS files ("
    "  path TEXT,"
    "  hash INTEGER,"
    "  size INTEGER,"
    "  UNIQUE(path, hash, size));"
    "CREATE TABLE IF NOT EXISTS blobs ("
    "  hash INTEGER,"
    "  size INTEGER,"
    "  blob BLOB,"
    "  UNIQUE(hash, size));"
    "CREATE TABLE IF NOT EXISTS file_blobs ("
    "  file_hash INTEGER,"
    "  blob_hash INTEGER,"
    "  ordinal INTEGER,"
    "  UNIQUE(file_hash, blob_hash, ordinal));"
    ;
static const char * const ADD_FILE =
    "INSERT OR IGNORE INTO files (path, hash, size)"
    " VALUES(?, ?, ?)";
static const char * const ADD_BLOB =
    "INSERT OR IGNORE INTO blobs (hash, size, blob)"
    " VALUES(?, ?, ?)";
static const char * const ADD_FILE_BLOB =
    "INSERT OR IGNORE INTO file_blobs (file_hash, blob_hash, ordinal)"
    " VALUES(?, ?, ?)";

struct node {
    Fnv64_t hash;
    int ordinal;
    struct node * prev;
};

struct node * new_node(Fnv64_t hash, int ordinal, struct node * prev)
{
    struct node * n = malloc(sizeof(struct node));
    n->hash = hash;
    n->ordinal = ordinal;
    n->prev = prev;
    return n;
}

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

void
insert_blob(Fnv64_t hash, int size, const char * const buf)
{
    sqlite3_stmt * stmt;

    if(SQLITE_OK == sqlite3_prepare_v2(DB, ADD_BLOB, -1, &stmt, NULL)) {
        if(SQLITE_OK == sqlite3_bind_int64(stmt, 1, hash)) {
            if(SQLITE_OK == sqlite3_bind_int(stmt, 2, size)) {
                if(SQLITE_OK == sqlite3_bind_blob(stmt, 3, buf, size, SQLITE_STATIC)) {
                    if(SQLITE_DONE == sqlite3_step(stmt)) {
                        // SUCCESS
                    }
                }
            }
        }
    }

    sqlite3_finalize(stmt);
}

void
insert_file( const char * const path, Fnv64_t hash, int size)
{
    sqlite3_stmt * stmt;

    if(SQLITE_OK == sqlite3_prepare_v2(DB, ADD_FILE, -1, &stmt, NULL)) {
        if(SQLITE_OK == sqlite3_bind_text(stmt, 1, path, strnlen(path, 1024),
                                          SQLITE_STATIC)) {
            if(SQLITE_OK == sqlite3_bind_int64(stmt, 2, hash)) {
                if(SQLITE_OK == sqlite3_bind_int(stmt, 3, size)) {
                    if(SQLITE_DONE == sqlite3_step(stmt)) {
                        // SUCCESS
                    }
                }
            }
        }
    }

    sqlite3_finalize(stmt);
}

void
insert_file_blob(Fnv64_t file_hash, Fnv64_t blob_hash, int ordinal)
{
    sqlite3_stmt * stmt;

    if(SQLITE_OK == sqlite3_prepare_v2(DB, ADD_FILE_BLOB, -1, &stmt, NULL)) {
        if(SQLITE_OK == sqlite3_bind_int64(stmt, 1, file_hash)) {
            if(SQLITE_OK == sqlite3_bind_int64(stmt, 2, blob_hash)) {
                if(SQLITE_OK == sqlite3_bind_int(stmt, 3, ordinal)) {
                    if(SQLITE_DONE == sqlite3_step(stmt)) {
                        // SUCCESS
                    }
                }
            }
        }
    }

    sqlite3_finalize(stmt);
}

unsigned long long int
fnv_hash(const char * const fp, const double len)
{
    FILE * fd = 0;
    const size_t max = len < MAX_LEN ? len : MAX_LEN;
    size_t read = 0;
    short ordinal = 0;
    Fnv64_t hash = FNV1A_64_INIT;
    struct node * root = 0;

    if ((fd = fopen(fp, "rb")) == NULL) {
        fprintf(stderr, "Can't open file %s; %s\n", fp, strerror(errno));
        fclose(fd);
        return 0;
    };

    char * buf = malloc(max);

    while (0 != (read = fread(buf,  1, max, fd))) {
        Fnv64_t blob_hash = FNV1A_64_INIT;
        blob_hash = fnv_64a_buf(buf, read, blob_hash);
        insert_blob(blob_hash, read, buf);
        root = new_node(blob_hash, ordinal++, root);
        hash = fnv_64a_buf(buf, read, hash);
    }

    insert_file(fp, hash, len);

    while(0 != root) {
        insert_file_blob(hash, root->hash, root->ordinal);
        root = root->prev;
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
