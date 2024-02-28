#include <stdio.h>
#include <stdlib.h>

#include "lexer.h"
#include "utils.h"

int main(int argc, char **argv) {
    if (argc != 2) {
        err("Usage: %s <source_file>", argv[0]);
    }

    char *file_buf = loadFile(argv[1]);
    Token *tokens = tokenize(file_buf);
    free(file_buf);
    showTokens(tokens);

    return 0;
}