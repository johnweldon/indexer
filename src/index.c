#define _XOPEN_SOURCE 700

#include <errno.h>
#include <ftw.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>

#include "index.h"
#include "fnv/fnv.h"
#include "sqlite/sqlite3.h"

sqlite3 * DB = 0;
char * db_name = 0;
char * sql_file = 0;
char * root_dir = 0;


const size_t MAX_PATH = 4096;
const size_t MAX_LEN = 1 << 30;
const char * const INIT_DB =
    "CREATE TABLE IF NOT EXISTS files ("
    " path TEXT,"
    " hash INTEGER,"
    " size INTEGER,"
    " UNIQUE(path, hash, size));"
    "CREATE TABLE IF NOT EXISTS blobs ("
    " hash INTEGER,"
    " size INTEGER,"
    " blob BLOB,"
    " UNIQUE(hash, size));"
    "CREATE TABLE IF NOT EXISTS file_blobs ("
    " file_hash INTEGER,"
    " blob_hash INTEGER,"
    " ordinal INTEGER,"
    " UNIQUE(file_hash, blob_hash, ordinal));"
    "CREATE TABLE IF NOT EXISTS file_tags ("
    " file_hash INTEGER,"
    " tag_key TEXT,"
    " tag_val TEXT,"
    " UNIQUE(file_hash, tag_key, tag_val));"
    ;
const char * const ADD_FILE =
    "INSERT OR IGNORE INTO files (path, hash, size)"
    " VALUES(?, ?, ?)"
    ;
const char * const ADD_BLOB =
    "INSERT OR IGNORE INTO blobs (hash, size, blob)"
    " VALUES(?, ?, ?)"
    ;
const char * const ADD_FILE_BLOB =
    "INSERT OR IGNORE INTO file_blobs (file_hash, blob_hash, ordinal)"
    " VALUES(?, ?, ?)"
    ;
const char * const ADD_FILE_TAG =
    "INSERT OR IGNORE INTO file_tags (file_hash, tag_key, tag_val)"
    " VALUES(?, ?, ?)"
    ;


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

int
db_result_handler(void * notUsed, int argc, char ** argv, char ** azColName)
{
    int i;

    for(i = 0; i < argc; i++) {
        fprintf(stdout, "%s%s",
                argv[i] ? argv[i] : "NULL",
                argc == (i + 1) ? "" : ",");
    }

    fprintf(stdout, "\n");
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
        if(SQLITE_OK == sqlite3_bind_text(stmt, 1, path, strnlen(path, MAX_PATH),
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

void
insert_file_tag(Fnv64_t file_hash, const char * const key,
                const char * const val)
{
    sqlite3_stmt * stmt;

    if(SQLITE_OK == sqlite3_prepare_v2(DB, ADD_FILE_TAG, -1, &stmt, NULL)) {
        if(SQLITE_OK == sqlite3_bind_int64(stmt, 1, file_hash)) {
            if(SQLITE_OK == sqlite3_bind_text(stmt, 2, key, strnlen(key, MAX_PATH),
                                              SQLITE_STATIC)) {
                if(SQLITE_OK == sqlite3_bind_text(stmt, 3, val, strnlen(val, MAX_PATH),
                                                  SQLITE_STATIC)) {
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
store_file(const char * const fp, const double len)
{
    FILE * fd = 0;
    const size_t max = len < MAX_LEN ? len : MAX_LEN;
    size_t read = 0;
    size_t total = 0;
    short ordinal = 0;
    Fnv64_t hash = FNV1A_64_INIT;
    struct node * root = 0;
    //fprintf(stderr, "\t\t\t\(%s) %lu/%f\n", fp, max, len);

    if ((fd = fopen(fp, "rb")) == NULL) {
        fprintf(stderr, "Can't open file %s; %s\n", fp, strerror(errno));
        return 0;
    };

    char * buf = malloc(max);

    while (0 != (read = fread(buf,  1, max, fd))) {
        if(total >= len) {
            fprintf(
                stderr,
                "\t\tfile %s was changed while being read; breaking...\n",
                fp);
            break;
        }

        Fnv64_t blob_hash = FNV1A_64_INIT;
        blob_hash = fnv_64a_buf(buf, read, blob_hash);
        insert_blob(blob_hash, read, buf);
        root = new_node(blob_hash, ordinal++, root);
        hash = fnv_64a_buf(buf, read, hash);
        total += read;
    }

    fclose(fd);
    char * bp = buf;

    if (0 == (buf = realloc(buf, MAX_PATH))) {
        free(bp);
        fprintf(stderr, "Can't realloc space... bailing\n");
        return 0;
    }

    bp = buf;
    size_t cwd_len = 0;

    if('/' != fp[0]) {
        if(getcwd(buf, MAX_PATH)) {
            cwd_len = strnlen(buf, MAX_PATH);

            if (cwd_len < MAX_PATH) {
                bp = buf + cwd_len + 1;
                buf[cwd_len] = '/';
            }
        }
    }

    strncpy(bp, fp, MAX_PATH - cwd_len - 1);
    char * fpcopy = realpath(buf, NULL);
    int fplen = strnlen(fpcopy, MAX_PATH);
    insert_file(fpcopy, hash, len);
    insert_file_tag(hash, "path", fpcopy);
    char * tag = "file";
    int has_ext = 0;

    for (; fplen >= 0; fplen--) {
        if(0 == has_ext && '.' == fpcopy[fplen]) {
            insert_file_tag(hash, "ext", fpcopy + fplen + 1);
            has_ext++;
        }

        if('/' == fpcopy[fplen]) {
            fpcopy[fplen] = '\0';
            insert_file_tag(hash, tag, fpcopy + fplen + 1);
            has_ext = -1;
            tag = "dir";
        }
    }

    if('\0' != fpcopy[0]) {
        insert_file_tag(hash, "dir", fpcopy);
    }

    free(fpcopy);
    fpcopy = 0;

    while(0 != root) {
        insert_file_blob(hash, root->hash, root->ordinal);
        struct node * prev = root->prev;
        free(root);
        root = prev;
    }

    free(buf);
    buf = 0;
    return hash;
}

static int
process_entry(const char * const fp, const struct stat * const info,
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
        fprintf(stdout, " ... %s (%0.0f) ", fp, bytes);

        if((hash = store_file(fp, bytes)) == 0) {
            hash = 0;
        };

        fprintf(stdout, "%llx\n", hash);

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

int
process_directory(const char * const dir)
{
    int result;

    if (NULL == dir || '\0' == *dir) {
        return errno  = EINVAL;
    }

    result = nftw(dir, process_entry, 15, FTW_PHYS) ;

    if (result >= 0) {
        errno = result;
    }

    return errno;
}
