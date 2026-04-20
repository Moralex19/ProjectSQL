#include "../include/storage.h"
#include "../include/transaction.h"
#include <dirent.h>

static char current_db[MAX_NAME_LEN] = "";

static void build_base_dir(char *out) {
    if (current_db[0] != '\0') {
        snprintf(out, MAX_PATH_LEN, "dbfiles/%s", current_db);
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

static void build_tmp_dir(char *out) {
    char base[MAX_PATH_LEN];
    build_base_dir(base);
    snprintf(out, MAX_PATH_LEN, "%s/.tmp", base);
}

int ensure_directories(void) {
    mkdir("dbfiles", 0777);
    mkdir("logs", 0777);
    if (current_db[0] != '\0') {
        char base[MAX_PATH_LEN];
        build_base_dir(base);
        mkdir(base, 0777);
    }
    char tmp[MAX_PATH_LEN];
    build_tmp_dir(tmp);
    mkdir(tmp, 0777);
    return 1;
}

int table_exists(const char *table_name) {
    char meta_path[MAX_PATH_LEN];
    char data_path[MAX_PATH_LEN];

    build_meta_path(table_name, meta_path);
    build_data_path(table_name, data_path);

    return access(meta_path, F_OK) == 0 && access(data_path, F_OK) == 0;
}

int read_table_columns(const char *table_name, char columns[][MAX_NAME_LEN], int *count) {
    char meta_path[MAX_PATH_LEN];
    build_meta_path(table_name, meta_path);

    FILE *fp = fopen(meta_path, "r");
    if (!fp) return 0;

    char line[MAX_LINE];
    if (!fgets(line, sizeof(line), fp)) {
        fclose(fp);
        return 0;
    }
    fclose(fp);

    *count = 0;
    char *token = strtok(line, "|\n\r");
    while (token && *count < MAX_COLUMNS) {
        strncpy(columns[*count], token, MAX_NAME_LEN - 1);
        columns[*count][MAX_NAME_LEN - 1] = '\0';
        (*count)++;
        token = strtok(NULL, "|\n\r");
    }

    return (*count > 0);
}

int create_table(const ParsedCommand *cmd) {
    if (table_exists(cmd->table)) {
        printf("La tabla '%s' ya existe.\n", cmd->table);
        return 0;
    }

    char meta_path[MAX_PATH_LEN];
    char data_path[MAX_PATH_LEN];
    build_meta_path(cmd->table, meta_path);
    build_data_path(cmd->table, data_path);

    FILE *meta = fopen(meta_path, "w");
    if (!meta) {
        perror("Error creando archivo meta");
        return 0;
    }

    for (int i = 0; i < cmd->column_count; i++) {
        fprintf(meta, "%s", cmd->columns[i]);
        if (i < cmd->column_count - 1) fprintf(meta, "|");
    }
    fprintf(meta, "\n");
    fclose(meta);

    FILE *data = fopen(data_path, "w");
    if (!data) {
        perror("Error creando archivo data");
        return 0;
    }
    fclose(data);

    printf("Tabla '%s' creada correctamente.\n", cmd->table);
    return 1;
}

int drop_table(const char *table_name) {
    if (!table_exists(table_name)) {
        printf("La tabla '%s' no existe.\n", table_name);
        return 0;
    }

    char meta_path[MAX_PATH_LEN];
    char data_path[MAX_PATH_LEN];
    build_meta_path(table_name, meta_path);
    build_data_path(table_name, data_path);

    if (transaction_is_active()) {
        transaction_backup_table(table_name);
    }

    remove(meta_path);
    remove(data_path);

    printf("Tabla '%s' eliminada.\n", table_name);
    return 1;
}

int insert_record(const ParsedCommand *cmd) {
    if (!table_exists(cmd->table)) {
        printf("La tabla '%s' no existe.\n", cmd->table);
        return 0;
    }

    char columns[MAX_COLUMNS][MAX_NAME_LEN];
    int column_count = 0;

    if (!read_table_columns(cmd->table, columns, &column_count)) {
        printf("No se pudieron leer las columnas.\n");
        return 0;
    }

    if (column_count != cmd->value_count) {
        printf("Cantidad de valores incorrecta. Esperados: %d, recibidos: %d\n",
               column_count, cmd->value_count);
        return 0;
    }

    if (transaction_is_active()) {
        transaction_backup_table(cmd->table);
    }

    char data_path[MAX_PATH_LEN];
    build_data_path(cmd->table, data_path);

    FILE *fp = fopen(data_path, "a");
    if (!fp) {
        perror("Error abriendo archivo data");
        return 0;
    }

    for (int i = 0; i < cmd->value_count; i++) {
        fprintf(fp, "%s", cmd->values[i]);
        if (i < cmd->value_count - 1) fprintf(fp, "|");
    }
    fprintf(fp, "\n");
    fclose(fp);

    printf("Registro insertado en '%s'.\n", cmd->table);
    return 1;
}

static int find_column_index(char columns[][MAX_NAME_LEN], int count, const char *target) {
    for (int i = 0; i < count; i++) {
        if (strcmp(columns[i], target) == 0) return i;
    }
    return -1;
}

int select_records(const ParsedCommand *cmd) {
    if (!table_exists(cmd->table)) {
        printf("La tabla '%s' no existe.\n", cmd->table);
        return 0;
    }

    char data_path[MAX_PATH_LEN];
    build_data_path(cmd->table, data_path);

    char columns[MAX_COLUMNS][MAX_NAME_LEN];
    int column_count = 0;

    if (!read_table_columns(cmd->table, columns, &column_count)) {
        printf("No se pudieron leer las columnas.\n");
        return 0;
    }

    int where_index = -1;
    if (cmd->has_where) {
        where_index = find_column_index(columns, column_count, cmd->where_column);
        if (where_index < 0) {
            printf("La columna WHERE '%s' no existe.\n", cmd->where_column);
            return 0;
        }
    }

    for (int i = 0; i < column_count; i++) {
        printf("%s", columns[i]);
        if (i < column_count - 1) printf(" | ");
    }
    printf("\n");

    FILE *fp = fopen(data_path, "r");
    if (!fp) {
        perror("Error leyendo data");
        return 0;
    }

    char line[MAX_LINE];
    int encontrados = 0;

    while (fgets(line, sizeof(line), fp)) {
        char buffer[MAX_LINE];
        strncpy(buffer, line, sizeof(buffer) - 1);
        buffer[sizeof(buffer) - 1] = '\0';

        char values[MAX_VALUES][MAX_VALUE_LEN];
        int value_count = 0;

        char *token = strtok(buffer, "|\n\r");
        while (token && value_count < MAX_VALUES) {
            strncpy(values[value_count], token, MAX_VALUE_LEN - 1);
            values[value_count][MAX_VALUE_LEN - 1] = '\0';
            value_count++;
            token = strtok(NULL, "|\n\r");
        }

        if (cmd->has_where) {
            if (where_index >= value_count) continue;
            if (strcmp(values[where_index], cmd->where_value) != 0) continue;
        }

        for (int i = 0; i < value_count; i++) {
            printf("%s", values[i]);
            if (i < value_count - 1) printf(" | ");
        }
        printf("\n");
        encontrados++;
    }

    fclose(fp);
    printf("Registros encontrados: %d\n", encontrados);
    return 1;
}

int update_records(const ParsedCommand *cmd) {
    if (!table_exists(cmd->table)) {
        printf("La tabla '%s' no existe.\n", cmd->table);
        return 0;
    }

    char columns[MAX_COLUMNS][MAX_NAME_LEN];
    int column_count = 0;

    if (!read_table_columns(cmd->table, columns, &column_count)) {
        printf("No se pudieron leer las columnas.\n");
        return 0;
    }

    int set_index = find_column_index(columns, column_count, cmd->set_column);
    int where_index = find_column_index(columns, column_count, cmd->where_column);

    if (set_index < 0) {
        printf("La columna SET '%s' no existe.\n", cmd->set_column);
        return 0;
    }
    if (where_index < 0) {
        printf("La columna WHERE '%s' no existe.\n", cmd->where_column);
        return 0;
    }

    if (transaction_is_active()) {
        transaction_backup_table(cmd->table);
    }

    char data_path[MAX_PATH_LEN];
    char temp_path[MAX_PATH_LEN];
    char tmp_dir[MAX_PATH_LEN];
    build_data_path(cmd->table, data_path);
    build_tmp_dir(tmp_dir);
    snprintf(temp_path, sizeof(temp_path), "%s/%s_update.tmp", tmp_dir, cmd->table);

    FILE *in = fopen(data_path, "r");
    FILE *out = fopen(temp_path, "w");

    if (!in || !out) {
        perror("Error abriendo archivos para update");
        if (in) fclose(in);
        if (out) fclose(out);
        return 0;
    }

    char line[MAX_LINE];
    int updated = 0;

    while (fgets(line, sizeof(line), in)) {
        char original[MAX_LINE];
        strncpy(original, line, sizeof(original) - 1);
        original[sizeof(original) - 1] = '\0';

        char values[MAX_VALUES][MAX_VALUE_LEN];
        int value_count = 0;

        char buffer[MAX_LINE];
        strncpy(buffer, line, sizeof(buffer) - 1);
        buffer[sizeof(buffer) - 1] = '\0';

        char *token = strtok(buffer, "|\n\r");
        while (token && value_count < MAX_VALUES) {
            strncpy(values[value_count], token, MAX_VALUE_LEN - 1);
            values[value_count][MAX_VALUE_LEN - 1] = '\0';
            value_count++;
            token = strtok(NULL, "|\n\r");
        }

        if (where_index < value_count && strcmp(values[where_index], cmd->where_value) == 0) {
            strncpy(values[set_index], cmd->set_value, MAX_VALUE_LEN - 1);
            values[set_index][MAX_VALUE_LEN - 1] = '\0';
            updated++;

            for (int i = 0; i < value_count; i++) {
                fprintf(out, "%s", values[i]);
                if (i < value_count - 1) fprintf(out, "|");
            }
            fprintf(out, "\n");
        } else {
            fputs(original, out);
        }
    }

    fclose(in);
    fclose(out);

    remove(data_path);
    rename(temp_path, data_path);

    printf("Registros actualizados: %d\n", updated);
    return 1;
}

int delete_records(const ParsedCommand *cmd) {
    if (!table_exists(cmd->table)) {
        printf("La tabla '%s' no existe.\n", cmd->table);
        return 0;
    }

    char columns[MAX_COLUMNS][MAX_NAME_LEN];
    int column_count = 0;

    if (!read_table_columns(cmd->table, columns, &column_count)) {
        printf("No se pudieron leer las columnas.\n");
        return 0;
    }

    int where_index = find_column_index(columns, column_count, cmd->where_column);
    if (where_index < 0) {
        printf("La columna WHERE '%s' no existe.\n", cmd->where_column);
        return 0;
    }

    if (transaction_is_active()) {
        transaction_backup_table(cmd->table);
    }

    char data_path[MAX_PATH_LEN];
    char temp_path[MAX_PATH_LEN];
    char tmp_dir[MAX_PATH_LEN];
    build_data_path(cmd->table, data_path);
    build_tmp_dir(tmp_dir);
    snprintf(temp_path, sizeof(temp_path), "%s/%s_delete.tmp", tmp_dir, cmd->table);

    FILE *in = fopen(data_path, "r");
    FILE *out = fopen(temp_path, "w");

    if (!in || !out) {
        perror("Error abriendo archivos para delete");
        if (in) fclose(in);
        if (out) fclose(out);
        return 0;
    }

    char line[MAX_LINE];
    int deleted = 0;

    while (fgets(line, sizeof(line), in)) {
        char buffer[MAX_LINE];
        strncpy(buffer, line, sizeof(buffer) - 1);
        buffer[sizeof(buffer) - 1] = '\0';

        char values[MAX_VALUES][MAX_VALUE_LEN];
        int value_count = 0;

        char *token = strtok(buffer, "|\n\r");
        while (token && value_count < MAX_VALUES) {
            strncpy(values[value_count], token, MAX_VALUE_LEN - 1);
            values[value_count][MAX_VALUE_LEN - 1] = '\0';
            value_count++;
            token = strtok(NULL, "|\n\r");
        }

        if (where_index < value_count && strcmp(values[where_index], cmd->where_value) == 0) {
            deleted++;
            continue;
        }

        fputs(line, out);
    }

    fclose(in);
    fclose(out);

    remove(data_path);
    rename(temp_path, data_path);

    printf("Registros eliminados: %d\n", deleted);
    return 1;
}

int create_database(const char *db_name) {
    char path[MAX_PATH_LEN];
    snprintf(path, MAX_PATH_LEN, "dbfiles/%s", db_name);

    if (access(path, F_OK) == 0) {
        printf("La base de datos '%s' ya existe.\n", db_name);
        return 0;
    }

    if (mkdir(path, 0777) != 0) {
        perror("Error creando base de datos");
        return 0;
    }

    char tmp_path[MAX_PATH_LEN];
    snprintf(tmp_path, MAX_PATH_LEN, "%s/.tmp", path);
    mkdir(tmp_path, 0777);

    printf("Base de datos '%s' creada correctamente.\n", db_name);
    return 1;
}

int use_database(const char *db_name) {
    char path[MAX_PATH_LEN];
    snprintf(path, MAX_PATH_LEN, "dbfiles/%s", db_name);

    if (access(path, F_OK) != 0) {
        printf("La base de datos '%s' no existe.\n", db_name);
        return 0;
    }

    strncpy(current_db, db_name, MAX_NAME_LEN - 1);
    current_db[MAX_NAME_LEN - 1] = '\0';

    ensure_directories();

    printf("Usando base de datos '%s'.\n", db_name);
    return 1;
}

const char *get_current_db(void) {
    if (current_db[0] == '\0') return NULL;
    return current_db;
}
