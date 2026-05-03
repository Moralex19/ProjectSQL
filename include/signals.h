#ifndef SIGNALS_H
#define SIGNALS_H

#include "common.h"

extern volatile sig_atomic_t g_running;

void setup_signal_handlers(void);
void trigger_exit_interrupt(void);

#endif

