#ifndef COMMON_H
#define COMMON_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <errno.h>

#define MAX_LINE 512
#define MAX_TOKENS 128
#define MAX_TOKEN_LEN 64
#define MAX_NAME_LEN 64
#define MAX_COLUMNS 32
#define MAX_VALUE_LEN 128
#define MAX_VALUES 32
#define MAX_PATH_LEN 256
#define MAX_TYPE_LEN 16

typedef enum {
    CMD_UNKNOWN = 0,
    CMD_CREATE,
    CMD_INSERT,
    CMD_SELECT,
    CMD_UPDATE,
    CMD_DELETE,
    CMD_DROP,
    CMD_BEGIN,
    CMD_COMMIT,
    CMD_ROLLBACK,
    CMD_EXIT,
    CMD_CREATE_DB,
    CMD_USE_DB
} CommandType;

typedef enum {
    TYPE_UNKNOWN = 0,
    TYPE_INT,
    TYPE_VARCHAR,
    TYPE_FLOAT
} DataType;

typedef struct {
    int count;
    char tokens[MAX_TOKENS][MAX_TOKEN_LEN];
} TokenList;

typedef struct {
    CommandType type;
    char table[MAX_NAME_LEN];
    char db_name[MAX_NAME_LEN];

    int column_count;
    char columns[MAX_COLUMNS][MAX_NAME_LEN];
    char column_types[MAX_COLUMNS][MAX_TYPE_LEN];

    int value_count;
    char values[MAX_VALUES][MAX_VALUE_LEN];

    char where_column[MAX_NAME_LEN];
    char where_value[MAX_VALUE_LEN];
    int has_where;

    char set_column[MAX_NAME_LEN];
    char set_value[MAX_VALUE_LEN];
    int has_set;
} ParsedCommand;

#endif
