#include <stdio.h>
#include <stdlib.h>

#include "utils.h"
#include "lexer.h"
#include "parser.h"
#include "ad.h"

#define TOKEN_LIST_FILE "test/token_list.txt"

int main(int argc, char **argv) {

    if (argc != 2) {
        err("Usage: %s <source_file.atomc>", argv[0]);
    }

    // Load source file and create streams for output
    char *file_buf = loadFile(argv[1]);
    FILE *token_list_stream = createOutputStream(TOKEN_LIST_FILE);

    // Lexer
    Token *tokens = tokenize(file_buf);
    free(file_buf);
    showTokens(tokens, token_list_stream);
    fclose(token_list_stream);

    // Create global domain
    pushDomain();

    // Parser
    parse(tokens);

    // Print gloal domain
    showDomain(symTable, "global");

    // Cleanup memory
    dropDomain();
    free(tokens);

    return 0;
}