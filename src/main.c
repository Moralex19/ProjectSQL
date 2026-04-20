#include "../include/common.h"
#include "../include/lexer.h"
#include "../include/parser.h"
#include "../include/executor.h"
#include "../include/storage.h"
#include "../include/signals.h"

int main(void) {
    char line[MAX_LINE];
    TokenList tokens;
    ParsedCommand cmd;
    int result;

    ensure_directories();
    setup_signal_handlers();

    printf("=====================================\n");
    printf(" Bienvenido a manGUI\n");
    printf(" Mini motor de base de datos en C\n");
    printf(" 'crt db <nombre>' para crear BD\n");
    printf(" 'us <nombre>' para usar BD\n");
    printf(" 'salir' para terminar\n");
    printf("=====================================\n");

    while (1) {
        const char *db = get_current_db();
        if (db != NULL) {
            printf("manGUI [%s] > ", db);
        } else {
            printf("manGUI > ");
        }

        if (!fgets(line, sizeof(line), stdin)) {
            printf("\nFin de entrada.\n");
            break;
        }

        line[strcspn(line, "\n")] = '\0';

        if (strlen(line) == 0) {
            continue;
        }

        tokenize_input(line, &tokens);

        if (!parse_command(&tokens, &cmd)) {
            printf("Error de sintaxis o comando inválido.\n");
            continue;
        }

        result = execute_command(&cmd);

        if (result == 2) {
            break;
        }
    }

    return 0;
}
