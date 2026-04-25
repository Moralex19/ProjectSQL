#include "../include/signals.h"
#include "../include/transaction.h"

volatile sig_atomic_t g_running = 1;

static void handle_sigint(int sig) {
    (void)sig;
    printf("\nSe recibió Ctrl + C.\n");
    if (transaction_is_active()) {
        printf("Hay una transacción activa. Se hará rollback automático.\n");
        transaction_rollback();
    }
    printf("Saliendo de forma segura...\n");
    _exit(0);
}

void setup_signal_handlers(void) {
    signal(SIGINT, handle_sigint);
}
