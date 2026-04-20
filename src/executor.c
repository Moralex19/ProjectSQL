#include "../include/executor.h"
#include "../include/storage.h"
#include "../include/transaction.h"
#include "../include/process.h"

int execute_command(const ParsedCommand *cmd) {
    switch (cmd->type) {
        case CMD_CREATE:
            return create_table(cmd);

        case CMD_INSERT:
            return insert_record(cmd);

        case CMD_SELECT:
            run_select_in_child(cmd, select_records);
            return 1;

        case CMD_UPDATE:
            return update_records(cmd);

        case CMD_DELETE:
            return delete_records(cmd);

        case CMD_DROP:
            return drop_table(cmd->table);

        case CMD_BEGIN:
            return transaction_begin();

        case CMD_COMMIT:
            return transaction_commit();

        case CMD_ROLLBACK:
            return transaction_rollback();

        case CMD_EXIT:
            printf("Saliendo de ZacariasDB...\n");
            return 2;

        case CMD_CREATE_DB:
            return create_database(cmd->db_name);

        case CMD_USE_DB:
            return use_database(cmd->db_name);

        default:
            printf("Comando no reconocido.\n");
            return 0;
    }
}
