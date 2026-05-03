#include "../include/parser.h"
#include "../include/dictionary.h"

static CommandType get_command_type(const char *token) {
    if (strcmp(token, KW_CREATE) == 0)   return CMD_CREATE;
    if (strcmp(token, KW_INSERT) == 0)   return CMD_INSERT;
    if (strcmp(token, KW_SELECT) == 0)   return CMD_SELECT;
    if (strcmp(token, KW_UPDATE) == 0)   return CMD_UPDATE;
    if (strcmp(token, KW_DELETE) == 0)   return CMD_DELETE;
    if (strcmp(token, KW_DROP) == 0)     return CMD_DROP;
    if (strcmp(token, KW_BEGIN) == 0)    return CMD_BEGIN;
    if (strcmp(token, KW_COMMIT) == 0)   return CMD_COMMIT;
    if (strcmp(token, KW_ROLLBACK) == 0) return CMD_ROLLBACK;
    if (strcmp(token, KW_EXIT) == 0)     return CMD_EXIT;
    if (strcmp(token, KW_USE) == 0)      return CMD_USE_DB;
    if (strcmp(token, KW_SHOW) == 0)     return CMD_SHOW_DBS;  /* default, refined below */
    return CMD_UNKNOWN;
}

static void init_command(ParsedCommand *cmd) {
    memset(cmd, 0, sizeof(ParsedCommand));
    cmd->type = CMD_UNKNOWN;
}

/* Verifica que el último token sea ";" */
static int has_semicolon(const TokenList *tokens) {
    if (tokens->count < 1) return 0;
    return strcmp(tokens->tokens[tokens->count - 1], ";") == 0;
}

/* Valida si un string es un tipo de dato soportado */
static int is_valid_type(const char *type) {
    if (strcmp(type, "int") == 0)     return 1;
    if (strcmp(type, "varchar") == 0) return 1;
    if (strcmp(type, "float") == 0)   return 1;
    return 0;
}

/*
 * Parsea una lista tipo (col1:int, col2:varchar, col3:float)
 * Separa nombre y tipo por el carácter ':'
 * Retorna el índice del ')' o -1 en error
 */
static int parse_column_definitions(const TokenList *tokens, int start,
                                    char names[][MAX_NAME_LEN],
                                    char types[][MAX_TYPE_LEN],
                                    int *count) {
    int i = start;
    *count = 0;

    if (i >= tokens->count || strcmp(tokens->tokens[i], "(") != 0) {
        return -1;
    }
    i++;

    while (i < tokens->count && strcmp(tokens->tokens[i], ")") != 0) {
        if (strcmp(tokens->tokens[i], ",") == 0) {
            i++;
            continue;
        }

        /* El token debe tener formato "nombre:tipo" */
        const char *tok = tokens->tokens[i];
        const char *colon = strchr(tok, ':');

        if (colon == NULL) {
            printf("Error: columna '%s' sin tipo de dato. Use formato nombre:tipo (ej: id:int)\n", tok);
            return -1;
        }

        int name_len = (int)(colon - tok);
        if (name_len <= 0 || name_len >= MAX_NAME_LEN) {
            printf("Error: nombre de columna inválido en '%s'.\n", tok);
            return -1;
        }

        strncpy(names[*count], tok, name_len);
        names[*count][name_len] = '\0';

        const char *type_str = colon + 1;
        if (!is_valid_type(type_str)) {
            printf("Error: tipo de dato '%s' no soportado. Use: int, varchar, float\n", type_str);
            return -1;
        }

        strncpy(types[*count], type_str, MAX_TYPE_LEN - 1);
        types[*count][MAX_TYPE_LEN - 1] = '\0';

        (*count)++;
        i++;
    }

    if (i >= tokens->count || strcmp(tokens->tokens[i], ")") != 0) {
        return -1;
    }

    return i;
}

static int
parse_parenthesized_values(const TokenList *tokens, int start, char output[][MAX_VALUE_LEN], int *count) {
    int i = start;
    *count = 0;

    if (i >= tokens->count || strcmp(tokens->tokens[i], "(") != 0) {
        return -1;
    }
    i++;

    while (i < tokens->count && strcmp(tokens->tokens[i], ")") != 0) {
        if (strcmp(tokens->tokens[i], ",") != 0) {
            strncpy(output[*count], tokens->tokens[i], MAX_VALUE_LEN - 1);
            output[*count][MAX_VALUE_LEN - 1] = '\0';
            (*count)++;
        }
        i++;
    }

    if (i >= tokens->count || strcmp(tokens->tokens[i], ")") != 0) {
        return -1;
    }

    return i;
}

int parse_command(const TokenList *tokens, ParsedCommand *cmd) {
    int i;
    init_command(cmd);

    if (tokens->count == 0) return 0;

    cmd->type = get_command_type(tokens->tokens[0]);

    switch (cmd->type) {
        case CMD_CREATE:
            if (tokens->count < 3) return 0;

            /* crt db <nombre>; */
            if (strcmp(tokens->tokens[1], KW_DB) == 0) {
                if (!has_semicolon(tokens)) {
                    printf("Error: falta ';' al final del comando.\n");
                    return 0;
                }
                cmd->type = CMD_CREATE_DB;
                strncpy(cmd->db_name, tokens->tokens[2], MAX_NAME_LEN - 1);
                cmd->db_name[MAX_NAME_LEN - 1] = '\0';
                return 1;
            }

            /* crt tbl <nombre> (col1:tipo, col2:tipo); */
            if (tokens->count < 5) return 0;
            if (strcmp(tokens->tokens[1], KW_TABLE) != 0) return 0;

            if (!has_semicolon(tokens)) {
                printf("Error: falta ';' al final del comando.\n");
                return 0;
            }

            strncpy(cmd->table, tokens->tokens[2], MAX_NAME_LEN - 1);
            cmd->table[MAX_NAME_LEN - 1] = '\0';

            i = parse_column_definitions(tokens, 3, cmd->columns, cmd->column_types, &cmd->column_count);
            if (i < 0) return 0;
            return 1;

        case CMD_INSERT:
            if (tokens->count < 6) return 0;
            if (strcmp(tokens->tokens[1], KW_INTO) != 0) return 0;

            if (!has_semicolon(tokens)) {
                printf("Error: falta ';' al final del comando.\n");
                return 0;
            }

            strncpy(cmd->table, tokens->tokens[2], MAX_NAME_LEN - 1);
            cmd->table[MAX_NAME_LEN - 1] = '\0';

            if (strcmp(tokens->tokens[3], KW_VALUES) != 0) return 0;

            i = parse_parenthesized_values(tokens, 4, cmd->values, &cmd->value_count);
            if (i < 0) return 0;
            return 1;

        case CMD_SELECT:
            if (tokens->count < 3) return 0;
            if (strcmp(tokens->tokens[1], KW_FROM) != 0) return 0;

            if (!has_semicolon(tokens)) {
                printf("Error: falta ';' al final del comando.\n");
                return 0;
            }

            strncpy(cmd->table, tokens->tokens[2], MAX_NAME_LEN - 1);
            cmd->table[MAX_NAME_LEN - 1] = '\0';

            /* sel de tabla; => 4 tokens (sel, de, tabla, ;) */
            /* sel de tabla donde col = val; => 8 tokens */
            if (tokens->count > 4) {
                if (tokens->count != 8) return 0;
                if (strcmp(tokens->tokens[3], KW_WHERE) != 0) return 0;
                strncpy(cmd->where_column, tokens->tokens[4], MAX_NAME_LEN - 1);
                strncpy(cmd->where_value, tokens->tokens[6], MAX_VALUE_LEN - 1);
                cmd->has_where = 1;
            }
            return 1;

        case CMD_UPDATE:
            /* act en tabla poner col = val donde col = val; => 11 tokens */
            if (tokens->count < 11) return 0;
            if (strcmp(tokens->tokens[1], KW_INTO) != 0) return 0;

            if (!has_semicolon(tokens)) {
                printf("Error: falta ';' al final del comando.\n");
                return 0;
            }

            strncpy(cmd->table, tokens->tokens[2], MAX_NAME_LEN - 1);
            cmd->table[MAX_NAME_LEN - 1] = '\0';

            if (strcmp(tokens->tokens[3], KW_SET) != 0) return 0;

            strncpy(cmd->set_column, tokens->tokens[4], MAX_NAME_LEN - 1);
            strncpy(cmd->set_value, tokens->tokens[6], MAX_VALUE_LEN - 1);
            cmd->has_set = 1;

            if (strcmp(tokens->tokens[7], KW_WHERE) != 0) return 0;

            strncpy(cmd->where_column, tokens->tokens[8], MAX_NAME_LEN - 1);
            strncpy(cmd->where_value, tokens->tokens[10], MAX_VALUE_LEN - 1);
            cmd->has_where = 1;
            return 1;

        case CMD_DELETE:
            /* bor de tabla donde col = val; => 8 tokens */
            if (tokens->count < 8) return 0;
            if (strcmp(tokens->tokens[1], KW_FROM) != 0) return 0;

            if (!has_semicolon(tokens)) {
                printf("Error: falta ';' al final del comando.\n");
                return 0;
            }

            strncpy(cmd->table, tokens->tokens[2], MAX_NAME_LEN - 1);
            cmd->table[MAX_NAME_LEN - 1] = '\0';

            if (strcmp(tokens->tokens[3], KW_WHERE) != 0) return 0;

            strncpy(cmd->where_column, tokens->tokens[4], MAX_NAME_LEN - 1);
            strncpy(cmd->where_value, tokens->tokens[6], MAX_VALUE_LEN - 1);
            cmd->has_where = 1;
            return 1;

        case CMD_DROP:
            /* elm tbl tabla; => 4 tokens */
            if (tokens->count < 4) return 0;
            if (strcmp(tokens->tokens[1], KW_TABLE) != 0) return 0;

            if (!has_semicolon(tokens)) {
                printf("Error: falta ';' al final del comando.\n");
                return 0;
            }

            strncpy(cmd->table, tokens->tokens[2], MAX_NAME_LEN - 1);
            cmd->table[MAX_NAME_LEN - 1] = '\0';
            return 1;

        case CMD_BEGIN:
        case CMD_COMMIT:
        case CMD_ROLLBACK:
            if (!has_semicolon(tokens)) {
                printf("Error: falta ';' al final del comando.\n");
                return 0;
            }
            return 1;

        case CMD_EXIT:
            /* 'salir' no requiere ';' */
            return 1;

        case CMD_USE_DB:
            /* us nombre; => 3 tokens */
            if (tokens->count < 3) return 0;

            if (!has_semicolon(tokens)) {
                printf("Error: falta ';' al final del comando.\n");
                return 0;
            }

            strncpy(cmd->db_name, tokens->tokens[1], MAX_NAME_LEN - 1);
            cmd->db_name[MAX_NAME_LEN - 1] = '\0';
            return 1;

        case CMD_CREATE_DB:
            /* Already handled inside CMD_CREATE */
            return 0;

        case CMD_SHOW_DBS:
            /* mos db; => show databases  |  mos tbl; => show tables */
            if (tokens->count < 3) return 0;

            if (!has_semicolon(tokens)) {
                printf("Error: falta ';' al final del comando.\n");
                return 0;
            }

            if (strcmp(tokens->tokens[1], KW_DB) == 0) {
                cmd->type = CMD_SHOW_DBS;
                return 1;
            }
            if (strcmp(tokens->tokens[1], KW_TABLE) == 0) {
                cmd->type = CMD_SHOW_TABLES;
                return 1;
            }

            printf("Error: use 'mos db;' o 'mos tbl;'\n");
            return 0;

        case CMD_SHOW_TABLES:
            /* Already handled inside CMD_SHOW_DBS */
            return 0;

        default:
            return 0;
    }
}