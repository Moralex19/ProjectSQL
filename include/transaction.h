#ifndef TRANSACTION_H
#define TRANSACTION_H

#include "common.h"

int transaction_begin(void);
int transaction_commit(void);
int transaction_rollback(void);
int transaction_is_active(void);
int transaction_backup_table(const char *table_name);

#endif
