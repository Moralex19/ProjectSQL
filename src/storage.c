#include "../include/storage.h"
#include "../include/transaction.h"
#include <dirent.h>
#include <stdarg.h>

static char current_db[MAX_NAME_LEN] = "";

/*
 * ── Helpers de I/O con llamadas al sistema ──
 *
 * read_line_fd() : lee una línea usando read() en lugar de fgets()
 * write_str_fd() : escribe un string usando write() en lugar de fputs()
 * write_fmt_fd() : escribe texto formateado usando write() en lugar de fprintf()
 */
static ssize_t read_line_fd(int fd, char *buf, size_t max) {
    ssize_t i = 0;
    char c;
    while (i < (ssize_t)(max - 1)) {
        ssize_t n = read(fd, &c, 1);
        if (n <= 0) break;
        buf[i++] = c;
        if (c == '\n') break;
    }
    buf[i] = '\0';
    return i;
}

static ssize_t write_str_fd(int fd, const char *str) {
    return write(fd, str, strlen(str));
}

static ssize_t write_fmt_fd(int fd, const char *fmt, ...) {
    char buf[MAX_LINE];
    va_list args;
    va_start(args, fmt);
    int n = vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);
    if (n > 0) return write(fd, buf, n);
    return -1;
}

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

/* Usa open() + read() + close() en lugar de fopen/fgets/fclose */
int read_table_columns(const char *table_name, char columns[][MAX_NAME_LEN], int *count) {
    char meta_path[MAX_PATH_LEN];
    build_meta_path(table_name, meta_path);

    int fd = open(meta_path, O_RDONLY);
    if (fd == -1) return 0;

    char line[MAX_LINE];
    if (read_line_fd(fd, line, sizeof(line)) <= 0) {
        close(fd);
        return 0;
    }
    close(fd);

    *count = 0;
    char *token = strtok(line, "|\n\r");
    while (token && *count < MAX_COLUMNS) {
        /* El formato en .meta es "nombre:tipo", extraemos solo el nombre */
        char *colon = strchr(token, ':');
        if (colon) {
            int name_len = (int)(colon - token);
            if (name_len >= MAX_NAME_LEN) name_len = MAX_NAME_LEN - 1;
            strncpy(columns[*count], token, name_len);
            columns[*count][name_len] = '\0';
        } else {
            /* Compatibilidad con tablas antiguas sin tipo */
            strncpy(columns[*count], token, MAX_NAME_LEN - 1);
            columns[*count][MAX_NAME_LEN - 1] = '\0';
        }
        (*count)++;
        token = strtok(NULL, "|\n\r");
    }

    return (*count > 0);
}

/* Usa open() + read() + close() en lugar de fopen/fgets/fclose */
int read_table_meta(const char *table_name, char columns[][MAX_NAME_LEN], char types[][MAX_TYPE_LEN], int *count) {
    char meta_path[MAX_PATH_LEN];
    build_meta_path(table_name, meta_path);

    int fd = open(meta_path, O_RDONLY);
    if (fd == -1) return 0;

    char line[MAX_LINE];
    if (read_line_fd(fd, line, sizeof(line)) <= 0) {
        close(fd);
        return 0;
    }
    close(fd);

    *count = 0;
    char *token = strtok(line, "|\n\r");
    while (token && *count < MAX_COLUMNS) {
        char *colon = strchr(token, ':');
        if (colon) {
            int name_len = (int)(colon - token);
            if (name_len >= MAX_NAME_LEN) name_len = MAX_NAME_LEN - 1;
            strncpy(columns[*count], token, name_len);
            columns[*count][name_len] = '\0';

            strncpy(types[*count], colon + 1, MAX_TYPE_LEN - 1);
            types[*count][MAX_TYPE_LEN - 1] = '\0';
        } else {
            /* Compatibilidad con tablas antiguas sin tipo */
            strncpy(columns[*count], token, MAX_NAME_LEN - 1);
            columns[*count][MAX_NAME_LEN - 1] = '\0';
            strncpy(types[*count], "varchar", MAX_TYPE_LEN - 1);
            types[*count][MAX_TYPE_LEN - 1] = '\0';
        }
        (*count)++;
        token = strtok(NULL, "|\n\r");
    }

    return (*count > 0);
}

/* Valida si un valor es compatible con el tipo declarado */
static int validate_value_type(const char *value, const char *type) {
    if (strcmp(type, "int") == 0) {
        /* Verificar que todos los caracteres sean dígitos (opcionalmente con signo) */
        int start = 0;
        if (value[0] == '-' || value[0] == '+') start = 1;
        if (value[start] == '\0') return 0;  /* Solo el signo, sin dígitos */
        for (int i = start; value[i] != '\0'; i++) {
            if (!isdigit((unsigned char)value[i])) return 0;
        }
        return 1;
    }

    if (strcmp(type, "float") == 0) {
        int start = 0;
        int has_dot = 0;
        if (value[0] == '-' || value[0] == '+') start = 1;
        if (value[start] == '\0') return 0;
        for (int i = start; value[i] != '\0'; i++) {
            if (value[i] == '.') {
                if (has_dot) return 0;  /* Más de un punto */
                has_dot = 1;
            } else if (!isdigit((unsigned char)value[i])) {
                return 0;
            }
        }
        return 1;
    }

    if (strcmp(type, "varchar") == 0) {
        /* varchar acepta cualquier texto */
        return 1;
    }

    /* Tipo desconocido, aceptar por compatibilidad */
    return 1;
}

/* Usa open() + write() + close() en lugar de fopen/fprintf/fclose */
int create_table(const ParsedCommand *cmd) {
    if (table_exists(cmd->table)) {
        printf("La tabla '%s' ya existe.\n", cmd->table);
        return 0;
    }

    char meta_path[MAX_PATH_LEN];
    char data_path[MAX_PATH_LEN];
    build_meta_path(cmd->table, meta_path);
    build_data_path(cmd->table, data_path);

    int meta_fd = open(meta_path, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (meta_fd == -1) {
        perror("Error creando archivo meta");
        return 0;
    }

    /* Guardar formato nombre:tipo|nombre:tipo */
    for (int i = 0; i < cmd->column_count; i++) {
        if (cmd->column_types[i][0] != '\0') {
            write_fmt_fd(meta_fd, "%s:%s", cmd->columns[i], cmd->column_types[i]);
        } else {
            write_fmt_fd(meta_fd, "%s:varchar", cmd->columns[i]);
        }
        if (i < cmd->column_count - 1) write_str_fd(meta_fd, "|");
    }
    write_str_fd(meta_fd, "\n");
    close(meta_fd);

    int data_fd = open(data_path, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (data_fd == -1) {
        perror("Error creando archivo data");
        return 0;
    }
    close(data_fd);

    printf("Tabla '%s' creada correctamente.\n", cmd->table);

    /* Mostrar esquema */
    printf("  Columnas:\n");
    for (int i = 0; i < cmd->column_count; i++) {
        printf("    - %s (%s)\n", cmd->columns[i],
               cmd->column_types[i][0] ? cmd->column_types[i] : "varchar");
    }
    return 1;
}

/* Usa unlink() (llamada al sistema) en lugar de remove() */
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

    unlink(meta_path);
    unlink(data_path);

    printf("Tabla '%s' eliminada.\n", table_name);
    return 1;
}

int insert_record(const ParsedCommand *cmd) {
    if (!table_exists(cmd->table)) {
        printf("La tabla '%s' no existe.\n", cmd->table);
        return 0;
    }

    char columns[MAX_COLUMNS][MAX_NAME_LEN];
    char types[MAX_COLUMNS][MAX_TYPE_LEN];
    int column_count = 0;

    if (!read_table_meta(cmd->table, columns, types, &column_count)) {
        printf("No se pudieron leer las columnas.\n");
        return 0;
    }

    if (column_count != cmd->value_count) {
        printf("Cantidad de valores incorrecta. Esperados: %d, recibidos: %d\n",
               column_count, cmd->value_count);
        return 0;
    }

    /* Validar tipos de datos */
    for (int i = 0; i < cmd->value_count; i++) {
        if (!validate_value_type(cmd->values[i], types[i])) {
            printf("Error de tipo: columna '%s' es de tipo '%s', pero se recibió '%s'.\n",
                   columns[i], types[i], cmd->values[i]);
            return 0;
        }
    }

    if (transaction_is_active()) {
        transaction_backup_table(cmd->table);
    }

    /* Usa open() con O_APPEND + write() + close() */
    char data_path[MAX_PATH_LEN];
    build_data_path(cmd->table, data_path);

    int fd = open(data_path, O_WRONLY | O_CREAT | O_APPEND, 0644);
    if (fd == -1) {
        perror("Error abriendo archivo data");
        return 0;
    }

    for (int i = 0; i < cmd->value_count; i++) {
        write_str_fd(fd, cmd->values[i]);
        if (i < cmd->value_count - 1) write_str_fd(fd, "|");
    }
    write_str_fd(fd, "\n");
    close(fd);

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

    /* Usa open() + read() + close() para leer archivo de datos */
    int fd = open(data_path, O_RDONLY);
    if (fd == -1) {
        perror("Error leyendo data");
        return 0;
    }

    char line[MAX_LINE];
    int encontrados = 0;

    while (read_line_fd(fd, line, sizeof(line)) > 0) {
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

    close(fd);
    printf("Registros encontrados: %d\n", encontrados);
    return 1;
}

int update_records(const ParsedCommand *cmd) {
    if (!table_exists(cmd->table)) {
        printf("La tabla '%s' no existe.\n", cmd->table);
        return 0;
    }

    char columns[MAX_COLUMNS][MAX_NAME_LEN];
    char types[MAX_COLUMNS][MAX_TYPE_LEN];
    int column_count = 0;

    if (!read_table_meta(cmd->table, columns, types, &column_count)) {
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

    /* Validar que el nuevo valor sea compatible con el tipo de la columna SET */
    if (!validate_value_type(cmd->set_value, types[set_index])) {
        printf("Error de tipo: columna '%s' es de tipo '%s', pero se intenta poner '%s'.\n",
               cmd->set_column, types[set_index], cmd->set_value);
        return 0;
    }

    if (transaction_is_active()) {
        transaction_backup_table(cmd->table);
    }

    /* Usa open() + read() + write() + close() + unlink() */
    char data_path[MAX_PATH_LEN];
    char temp_path[MAX_PATH_LEN];
    char tmp_dir[MAX_PATH_LEN];
    build_data_path(cmd->table, data_path);
    build_tmp_dir(tmp_dir);
    snprintf(temp_path, sizeof(temp_path), "%s/%s_update.tmp", tmp_dir, cmd->table);

    int in_fd = open(data_path, O_RDONLY);
    int out_fd = open(temp_path, O_WRONLY | O_CREAT | O_TRUNC, 0644);

    if (in_fd == -1 || out_fd == -1) {
        perror("Error abriendo archivos para update");
        if (in_fd != -1) close(in_fd);
        if (out_fd != -1) close(out_fd);
        return 0;
    }

    char line[MAX_LINE];
    int updated = 0;

    while (read_line_fd(in_fd, line, sizeof(line)) > 0) {
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
                write_str_fd(out_fd, values[i]);
                if (i < value_count - 1) write_str_fd(out_fd, "|");
            }
            write_str_fd(out_fd, "\n");
        } else {
            write_str_fd(out_fd, original);
        }
    }

    close(in_fd);
    close(out_fd);

    unlink(data_path);
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

    /* Usa open() + read() + write() + close() + unlink() */
    char data_path[MAX_PATH_LEN];
    char temp_path[MAX_PATH_LEN];
    char tmp_dir[MAX_PATH_LEN];
    build_data_path(cmd->table, data_path);
    build_tmp_dir(tmp_dir);
    snprintf(temp_path, sizeof(temp_path), "%s/%s_delete.tmp", tmp_dir, cmd->table);

    int in_fd = open(data_path, O_RDONLY);
    int out_fd = open(temp_path, O_WRONLY | O_CREAT | O_TRUNC, 0644);

    if (in_fd == -1 || out_fd == -1) {
        perror("Error abriendo archivos para delete");
        if (in_fd != -1) close(in_fd);
        if (out_fd != -1) close(out_fd);
        return 0;
    }

    char line[MAX_LINE];
    int deleted = 0;

    while (read_line_fd(in_fd, line, sizeof(line)) > 0) {
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

        write_str_fd(out_fd, line);
    }

    close(in_fd);
    close(out_fd);

    unlink(data_path);
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

int show_databases(void) {
    DIR *dir = opendir("dbfiles");
    if (!dir) {
        printf("No se pudo acceder al directorio de bases de datos.\n");
        return 0;
    }

    struct dirent *entry;
    int count = 0;

    printf("Bases de datos disponibles:\n");
    printf("─────────────────────────\n");

    while ((entry = readdir(dir)) != NULL) {
        /* Ignorar '.', '..' y archivos ocultos */
        if (entry->d_name[0] == '.') continue;

        /* Verificar que sea un directorio */
        char path[MAX_PATH_LEN];
        snprintf(path, MAX_PATH_LEN, "dbfiles/%s", entry->d_name);

        struct stat st;
        if (stat(path, &st) == 0 && S_ISDIR(st.st_mode)) {
            const char *marker = "";
            if (current_db[0] != '\0' && strcmp(current_db, entry->d_name) == 0) {
                marker = "  <-- actual";
            }
            printf("  • %s%s\n", entry->d_name, marker);
            count++;
        }
    }

    closedir(dir);

    if (count == 0) {
        printf("  (ninguna)\n");
    }
    printf("─────────────────────────\n");
    printf("Total: %d base(s) de datos.\n", count);
    return 1;
}

int show_tables(void) {
    if (current_db[0] == '\0') {
        printf("No hay base de datos seleccionada. Usa 'us <nombre>;'\n");
        return 0;
    }

    char base[MAX_PATH_LEN];
    build_base_dir(base);

    DIR *dir = opendir(base);
    if (!dir) {
        printf("No se pudo acceder al directorio de la base de datos.\n");
        return 0;
    }

    struct dirent *entry;
    int count = 0;

    printf("Tablas en '%s':\n", current_db);
    printf("─────────────────────────\n");

    while ((entry = readdir(dir)) != NULL) {
        if (entry->d_name[0] == '.') continue;

        /* Solo archivos .meta representan tablas */
        const char *dot = strrchr(entry->d_name, '.');
        if (dot && strcmp(dot, ".meta") == 0) {
            /* Extraer el nombre de la tabla (sin .meta) */
            char tbl_name[MAX_NAME_LEN];
            int name_len = (int)(dot - entry->d_name);
            if (name_len >= MAX_NAME_LEN) name_len = MAX_NAME_LEN - 1;
            strncpy(tbl_name, entry->d_name, name_len);
            tbl_name[name_len] = '\0';

            /* Leer las columnas para mostrar el esquema */
            char columns[MAX_COLUMNS][MAX_NAME_LEN];
            char types[MAX_COLUMNS][MAX_TYPE_LEN];
            int col_count = 0;

            if (read_table_meta(tbl_name, columns, types, &col_count)) {
                printf("  • %s (", tbl_name);
                for (int i = 0; i < col_count; i++) {
                    printf("%s:%s", columns[i], types[i]);
                    if (i < col_count - 1) printf(", ");
                }
                printf(")\n");
            } else {
                printf("  • %s\n", tbl_name);
            }
            count++;
        }
    }

    closedir(dir);

    if (count == 0) {
        printf("  (ninguna)\n");
    }
    printf("─────────────────────────\n");
    printf("Total: %d tabla(s).\n", count);
    return 1;
}
