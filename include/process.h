#ifndef PROCESS_H
#define PROCESS_H

#include "common.h"

void log_action_async(const char *message);
void run_select_in_child(const ParsedCommand *cmd, int (*select_fn)(const ParsedCommand *));

#endif
