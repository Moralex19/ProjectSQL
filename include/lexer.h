#ifndef LEXER_H
#define LEXER_H

#include "common.h"

void tokenize_input(const char *input, TokenList *tokens);
void print_tokens(const TokenList *tokens);

#endif
