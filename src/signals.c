#include "../include/signals.h"
#include "../include/transaction.h"

volatile sig_atomic_t g_running = 1;

/* ── Handler para Ctrl+C (SIGINT) ── */
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

/* ── Handler para interrupción de salida (SIGUSR1) ──
 * Esta señal se dispara con raise() cuando el usuario escribe 'salir'.
 * Funciona como una interrupción por software: en lugar de retornar
 * normalmente del programa, se genera una señal que interrumpe la
 * ejecución y transfiere el control a este handler.
 */
static void handle_exit_signal(int sig) {
    (void)sig;

    /* Limpieza: si hay transacción activa, hacer rollback */
    if (transaction_is_active()) {
        printf("Transacción activa detectada. Ejecutando rollback automático...\n");
        transaction_rollback();
    }

    printf("Interrupción de salida (SIGUSR1) recibida. Cerrando manGUI...\n");

    /* Llamada al sistema _exit() para terminar el proceso */
    _exit(0);
}

void setup_signal_handlers(void) {
    /* Registrar handlers de señales (llamadas al sistema signal()) */
    signal(SIGINT, handle_sigint);    /* Ctrl+C          */
    signal(SIGUSR1, handle_exit_signal); /* Salida por 'salir' */
}

/*
 * trigger_exit_interrupt()
 *
 * Genera una interrupción por software enviando la señal SIGUSR1
 * al proceso actual usando la llamada al sistema raise().
 *
 * Esto simula una interrupción: el flujo normal del programa se
 * interrumpe y el control pasa al handler handle_exit_signal().
 *
 * Equivale a:  kill(getpid(), SIGUSR1);
 */
void trigger_exit_interrupt(void) {
    raise(SIGUSR1);
}
