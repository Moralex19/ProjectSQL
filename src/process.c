#include "../include/process.h"

void log_action_async(const char *message) {
    pid_t pid = fork();

    if (pid < 0) {
        perror("fork");
        return;
    }

    if (pid == 0) {
        FILE *fp = fopen("logs/actions.log", "a");
        if (fp) {
            fprintf(fp, "%s\n", message);
            fclose(fp);
        }
        exit(0);
    } else {
        waitpid(pid, NULL, 0);
    }
}

void run_select_in_child(const ParsedCommand *cmd, int (*select_fn)(const ParsedCommand *)) {
    pid_t pid = fork();

    if (pid < 0) {
        perror("fork");
        return;
    }

    if (pid == 0) {
        select_fn(cmd);
        exit(0);
    } else {
        waitpid(pid, NULL, 0);
    }
}
