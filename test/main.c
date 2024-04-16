#include <stdio.h>
#include <stdlib.h>

#include "utils.h"
#include "lexer.h"
#include "parser.h"

int main(int argc, char **argv) {

    if (argc != 2) {
        err("Usage: %s <source_file>", argv[0]);
    }

    char *file_buf = loadFile(argv[1]);

    // Lexer
    Token *tokens = tokenize(file_buf);
    free(file_buf);

    // Parser
    parse(tokens);

    return 0;
}