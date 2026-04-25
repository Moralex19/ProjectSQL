#ifndef STORAGE_H
#define STORAGE_H

#include "common.h"

int ensure_directories(void);
int table_exists(const char *table_name);

int create_table(const ParsedCommand *cmd);
int drop_table(const char *table_name);
int insert_record(const ParsedCommand *cmd);
int select_records(const ParsedCommand *cmd);
int update_records(const ParsedCommand *cmd);
int delete_records(const ParsedCommand *cmd);

int read_table_columns(const char *table_name, char columns[][MAX_NAME_LEN], int *count);
int read_table_meta(const char *table_name, char columns[][MAX_NAME_LEN], char types[][MAX_TYPE_LEN], int *count);

int create_database(const char *db_name);
int use_database(const char *db_name);
const char *get_current_db(void);

#endif
