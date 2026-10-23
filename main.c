#include <stdio.h>
#include "lexer.h"

static const char* typeName(TokenType t) {
    switch (t) {
        case KEYWORD:           return "Keyword";
        case OPERATOR:          return "Operator";
        case SPECIAL_CHARACTER: return "Special character";
        case CONSTANT:          return "Constant";
        case IDENTIFIER:        return "Identifier";
        case ARRAY:             return "Array";
        case PREPROCESSOR:       return "Preprocessor";
        default:                return "Unknown";
    }
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        printf("Usage: %s <source file>\n", argv[0]);
        return 1;
    }

    initializeLexer(argv[1]);

    Token token;
    int errors = 0;

    while ((token = getNextToken()).type != END_OF_FILE) {
        if (token.type == UNKNOWN) {
            printf("Error: invalid token '%s'\n", token.lexeme);
            errors++;
        } else {
            printf("%-18s : %s\n", typeName(token.type), token.lexeme);
        }
    }

    printf("\nTotal errors: %d\n", errors);
    return 0;
}