#include "../include/transaction.h"
#include "../include/storage.h"

static int g_transaction_active = 0;
static char backed_up_tables[64][MAX_NAME_LEN];
static int backup_count = 0;

static void build_base_dir(char *out) {
    const char *db = get_current_db();
    if (db != NULL) {
        snprintf(out, MAX_PATH_LEN, "dbfiles/%s", db);
    } else {
        snprintf(out, MAX_PATH_LEN, "dbfiles");
    }
}

static void build_meta_path(const char *table, char *out) {
    char base[MAX_PATH_LEN];
    build_base_dir(base);
    snprintf(out, MAX_PATH_LEN, "%s/%s.meta", base, table);
}

static void build_data_path(const char *table, char *out) {
    char base[MAX_PATH_LEN];
    build_base_dir(base);
    snprintf(out, MAX_PATH_LEN, "%s/%s.data", base, table);
}

static void build_backup_meta_path(const char *table, char *out) {
    char base[MAX_PATH_LEN];
    build_base_dir(base);
    snprintf(out, MAX_PATH_LEN, "%s/.tmp/%s.meta.bak", base, table);
}

static void build_backup_data_path(const char *table, char *out) {
    char base[MAX_PATH_LEN];
    build_base_dir(base);
    snprintf(out, MAX_PATH_LEN, "%s/.tmp/%s.data.bak", base, table);
}

static int copy_file(const char *src, const char *dst) {
    FILE *in = fopen(src, "rb");
    if (!in) return 0;

    FILE *out = fopen(dst, "wb");
    if (!out) {
        fclose(in);
        return 0;
    }

    char buffer[1024];
    size_t n;
    while ((n = fread(buffer, 1, sizeof(buffer), in)) > 0) {
        fwrite(buffer, 1, n, out);
    }

    fclose(in);
    fclose(out);
    return 1;
}

static int already_backed_up(const char *table_name) {
    for (int i = 0; i < backup_count; i++) {
        if (strcmp(backed_up_tables[i], table_name) == 0) return 1;
    }
    return 0;
}

int transaction_is_active(void) {
    return g_transaction_active;
}

int transaction_begin(void) {
    if (g_transaction_active) {
        printf("Ya hay una transacción activa.\n");
        return 0;
    }

    g_transaction_active = 1;
    backup_count = 0;
    printf("Transacción iniciada.\n");
    return 1;
}

int transaction_backup_table(const char *table_name) {
    if (!g_transaction_active) return 1;
    if (already_backed_up(table_name)) return 1;

    char meta[MAX_PATH_LEN], data[MAX_PATH_LEN];
    char bmeta[MAX_PATH_LEN], bdata[MAX_PATH_LEN];

    build_meta_path(table_name, meta);
    build_data_path(table_name, data);
    build_backup_meta_path(table_name, bmeta);
    build_backup_data_path(table_name, bdata);

    if (access(meta, F_OK) == 0) copy_file(meta, bmeta);
    if (access(data, F_OK) == 0) copy_file(data, bdata);

    strncpy(backed_up_tables[backup_count], table_name, MAX_NAME_LEN - 1);
    backed_up_tables[backup_count][MAX_NAME_LEN - 1] = '\0';
    backup_count++;

    return 1;
}

int transaction_commit(void) {
    if (!g_transaction_active) {
        printf("No hay transacción activa.\n");
        return 0;
    }

    for (int i = 0; i < backup_count; i++) {
        char bmeta[MAX_PATH_LEN], bdata[MAX_PATH_LEN];
        build_backup_meta_path(backed_up_tables[i], bmeta);
        build_backup_data_path(backed_up_tables[i], bdata);

        remove(bmeta);
        remove(bdata);
    }

    g_transaction_active = 0;
    backup_count = 0;

    printf("Commit realizado.\n");
    return 1;
}

int transaction_rollback(void) {
    if (!g_transaction_active) {
        printf("No hay transacción activa.\n");
        return 0;
    }

    for (int i = 0; i < backup_count; i++) {
        char meta[MAX_PATH_LEN], data[MAX_PATH_LEN];
        char bmeta[MAX_PATH_LEN], bdata[MAX_PATH_LEN];

        build_meta_path(backed_up_tables[i], meta);
        build_data_path(backed_up_tables[i], data);
        build_backup_meta_path(backed_up_tables[i], bmeta);
        build_backup_data_path(backed_up_tables[i], bdata);

        if (access(bmeta, F_OK) == 0) {
            copy_file(bmeta, meta);
            remove(bmeta);
        }
        if (access(bdata, F_OK) == 0) {
            copy_file(bdata, data);
            remove(bdata);
        }
    }

    g_transaction_active = 0;
    backup_count = 0;

    printf("Rollback realizado.\n");
    return 1;
}

