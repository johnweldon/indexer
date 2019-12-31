#define _XOPEN_SOURCE 700

#include <errno.h>
#include <ftw.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>

#include "main.h"
#include "index.h"
#include "fnv/fnv.h"
#include "sqlite/sqlite3.h"


int
main(int argc, char ** argv)
{
    int ch;

    while((ch = getopt(argc, argv, "d:q:r:")) != -1) {
        switch(ch) {
        case 'd':
            db_name = optarg;
            break;

        case 'q':
            sql_file = optarg;
            break;

        case 'r':
            root_dir = optarg;
            break;

        case '?':
            return(1);

        default:
            fprintf(stderr, "Unexpected option 0%o\n", ch);
            return(1);
        }
    }

    char * zErrMsg = 0;
    int rc = 0;

    if (
        0 == db_name ||
        (0 == sql_file && 0 == root_dir) ||
        (0 != sql_file && 0 != root_dir)) {
        fprintf(
            stderr,
            "Usage: %s -d <db> -r <root_dir>\n"
            "    or %s -d <db> -q <query_file>\n",
            argv[0],
            argv[0]);
        return(1);
    }

    rc = sqlite3_open(db_name, &DB);

    if(rc) {
        fprintf(stderr, "Can't open db %s; %s\n", db_name, sqlite3_errmsg(DB));
        sqlite3_close(DB);
        return(rc);
    }

    rc = sqlite3_exec(DB, INIT_DB, db_result_handler, 0, &zErrMsg);

    if(rc) {
        fprintf(stderr, "Can't initialize db %s; %s\n", db_name, zErrMsg);
        sqlite3_close(DB);
        return(rc);
    }

    if(0 != root_dir) {
        rc = process_directory(root_dir);

        if(rc) {
            fprintf(stderr, "Can't walk dir %s; %s\n", root_dir, strerror(errno));
            sqlite3_close(DB);
            return(rc);
        }
    }

    if(0 != sql_file) {
        struct stat fs = {0};

        if(stat(sql_file, &fs)) {
            fprintf(stderr, "Can't stat file %s; %s\n", sql_file, strerror(errno));
            return(1);
        }

        FILE * fd = 0;

        if ((fd = fopen(sql_file, "rb")) == NULL) {
            fprintf(stderr, "Can't open file %s; %s\n", sql_file, strerror(errno));
            return(1);
        };

        char * query = malloc(fs.st_size + 1);

        for(int i = 0; i <= fs.st_size; i++) {
            query[i] = '\0';
        }

        if(fread(query, 1, fs.st_size, fd) != fs.st_size ) {
            fprintf(stderr, "Can't read file %s; %s\n", sql_file, strerror(errno));
            free(query);
            fclose(fd);
            return(1);
        }

        rc = sqlite3_exec(DB, query, db_result_handler, 0, &zErrMsg);
        fclose(fd);
        free(query);

        if(rc) {
            fprintf(stderr, "Can't execute script %s; %s\n", sql_file, zErrMsg);
            sqlite3_close(DB);
            return(rc);
        }
    }

    sqlite3_close(DB);
    return(0);
}
