#include <stdio.h>
#include <stdlib.h>

#include "lexer.h"
#include "utils.h"

int main(int argc, char **argv) {

    if (argc != 2) {
        err("Usage: %s <source_file>", argv[0]);
    }

    char *file_buf = loadFile(argv[1]);
    printf("Source file content:\n=== BEGIN ===\n%s=== END ===\n", file_buf);

    Token *tokens = tokenize(file_buf);
    free(file_buf);

    freopen("test/atom_list.txt", "w", stdout);
    showTokens(tokens);

    return 0;
}