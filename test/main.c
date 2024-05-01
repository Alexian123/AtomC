#include <stdio.h>
#include <stdlib.h>

#include "utils.h"
#include "lexer.h"
#include "parser.h"
#include "ad.h"

#define TOKEN_LIST_FILE "test/token_list.txt"
#define GLOBAL_DOMAIN_FILE "test/global_domain.txt"

int main(int argc, char **argv) {

    if (argc != 2) {
        err("Usage: %s <source_file.atomc>", argv[0]);
    }

    // Load source file and create output streams
    char *file_buf = loadFile(argv[1]);
    FILE *token_list_stream = createOutputStream(TOKEN_LIST_FILE);
    FILE *global_domain_stream = createOutputStream(GLOBAL_DOMAIN_FILE);

    // Run lexer
    Token *tokens = tokenize(file_buf);
    free(file_buf);
    showTokens(tokens, token_list_stream);
    fclose(token_list_stream);

    // Create global domain
    pushDomain();

    // Run parser
    parse(tokens);

    // Show global domain
    showDomain(symTable, "global", global_domain_stream);
    fclose(global_domain_stream);

    // Cleanup memory
    dropDomain();
    freeTokens(tokens);

    return 0;
}