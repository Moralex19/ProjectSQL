#include "../include/process.h"

/*
 * log_action_async()
 *
 * Usa llamadas al sistema:
 *   - fork()  : crea proceso hijo
 *   - open()  : abre archivo con flags del sistema
 *   - write() : escribe bytes al descriptor de archivo
 *   - close() : cierra descriptor de archivo
 *   - _exit() : termina proceso hijo
 */
void log_action_async(const char *message) {
    pid_t pid = fork();

    if (pid < 0) {
        perror("fork");
        return;
    }

    if (pid == 0) {
        /* Proceso hijo: usar llamadas al sistema para escribir al log */
        int fd = open("logs/actions.log", O_WRONLY | O_CREAT | O_APPEND, 0644);
        if (fd != -1) {
            write(fd, message, strlen(message));
            write(fd, "\n", 1);
            close(fd);
        }
        _exit(0);
    } else {
        waitpid(pid, NULL, 0);
    }
}

/*
 * run_select_in_child()
 *
 * Usa llamadas al sistema:
 *   - fork()    : crea proceso hijo para ejecutar SELECT
 *   - waitpid() : proceso padre espera al hijo
 *   - _exit()   : termina proceso hijo
 */
void run_select_in_child(const ParsedCommand *cmd, int (*select_fn)(const ParsedCommand *)) {
    pid_t pid = fork();

    if (pid < 0) {
        perror("fork");
        return;
    }

    if (pid == 0) {
        select_fn(cmd);
        _exit(0);
    } else {
        waitpid(pid, NULL, 0);
    }
}
