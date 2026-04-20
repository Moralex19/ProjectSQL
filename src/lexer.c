#include "../include/lexer.h"

void tokenize_input(const char *input, TokenList *tokens) {
    tokens->count = 0;
    int i = 0;
    int len = (int)strlen(input);

    while (i < len) {
        while (i < len && isspace((unsigned char)input[i])) {
            i++;
        }

        if (i >= len) break;

        if (strchr("(),=", input[i]) != NULL) {
            if (tokens->count < MAX_TOKENS) {
                tokens->tokens[tokens->count][0] = input[i];
                tokens->tokens[tokens->count][1] = '\0';
                tokens->count++;
            }
            i++;
            continue;
        }

        int j = 0;
        char temp[MAX_TOKEN_LEN] = {0};

        while (i < len &&
               !isspace((unsigned char)input[i]) &&
               strchr("(),=", input[i]) == NULL) {
            if (j < MAX_TOKEN_LEN - 1) {
                temp[j++] = input[i];
            }
            i++;
        }
        temp[j] = '\0';

        if (tokens->count < MAX_TOKENS) {
            strncpy(tokens->tokens[tokens->count], temp, MAX_TOKEN_LEN - 1);
            tokens->tokens[tokens->count][MAX_TOKEN_LEN - 1] = '\0';
            tokens->count++;
        }
    }
}

void print_tokens(const TokenList *tokens) {
    printf("Tokens:\n");
    for (int i = 0; i < tokens->count; i++) {
        printf("[%d] %s\n", i, tokens->tokens[i]);
    }
}
