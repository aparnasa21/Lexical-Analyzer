#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "lexer.h"

static const char* keywords[MAX_KEYWORDS] = {
    "int", "float", "return", "if", "else", "while", "for", "do", "break", "continue",
    "char", "double", "void", "switch", "case", "default", "const", "static", "sizeof", "struct"
};

static const char* operators = "+-*/%=!<>|&.";
static const char* specialCharacters = ",;{}()[]";

static FILE *fp = NULL;

/* ---------- setup ---------- */

void initializeLexer(const char* filename) {
    fp = fopen(filename, "r");
    if (fp == NULL) {
        printf("Error: cannot open file %s\n", filename);
        exit(1);
    }
}

/* ---------- yes/no checks on a finished lexeme ---------- */

int isKeyword(const char* str) {
    for (int i = 0; i < MAX_KEYWORDS; i++) {
        if (strcmp(str, keywords[i]) == 0) {
            return 1;
        }
    }
    return 0;
}

int isSpecialCharacter(char ch) {
    return ch != '\0' && strchr(specialCharacters, ch) != NULL;
}

int isOperator(const char* str) {
    static const char* twoChar[] = {
        "==", "!=", "<=", ">=", "&&", "||", "++", "--", "+=", "-=", "*=", "/=",
        "->", "<<", ">>", "%=", "&=", "|="
    };
    static const char* threeChar[] = { "<<=", ">>=" };

    size_t len = strlen(str);

    if (len == 1) {
        return strchr(operators, str[0]) != NULL;
    }
    if (len == 2) {
        for (int i = 0; i < (int)(sizeof(twoChar) / sizeof(twoChar[0])); i++) {
            if (strcmp(str, twoChar[i]) == 0) {
                return 1;
            }
        }
    }
    if (len == 3) {
        for (int i = 0; i < (int)(sizeof(threeChar) / sizeof(threeChar[0])); i++) {
            if (strcmp(str, threeChar[i]) == 0) {
                return 1;
            }
        }
    }
    return 0;
}

int isIdentifier(const char* str) {
    if (!(isalpha((unsigned char)str[0]) || str[0] == '_')) {
        return 0;
    }
    for (int i = 1; str[i] != '\0'; i++) {
        if (!(isalnum((unsigned char)str[i]) || str[i] == '_')) {
            return 0;
        }
    }
    return 1;
}

/* integer suffix: u, l, ul, lu, ll, ull ... (u at most once, l at most twice) */
static int isIntSuffix(const char* s) {
    int u = 0, l = 0;
    for (; *s != '\0'; s++) {
        if (*s == 'u' || *s == 'U') {
            u++;
        } else if (*s == 'l' || *s == 'L') {
            l++;
        } else {
            return 0;
        }
    }
    return u <= 1 && l <= 2;
}

/* checks the whole text is one valid number: 42, 10L, 0x1F, 3.14, 1e-5, 1.5f */
static int isNumber(const char* s) {
    size_t i = 0;
    int digits = 0;

    /* hexadecimal: 0x or 0X followed by hex digits */
    if (s[0] == '0' && (s[1] == 'x' || s[1] == 'X')) {
        i = 2;
        while (isxdigit((unsigned char)s[i])) {
            i++;
            digits++;
        }
        return digits > 0 && isIntSuffix(s + i);
    }

    /* decimal: digits, optional .digits, optional exponent */
    int isFloat = 0;
    while (isdigit((unsigned char)s[i])) {
        i++;
        digits++;
    }
    if (s[i] == '.') {
        isFloat = 1;
        i++;
        while (isdigit((unsigned char)s[i])) {
            i++;
            digits++;
        }
    }
    if (digits == 0) {
        return 0;
    }
    if (s[i] == 'e' || s[i] == 'E') {
        isFloat = 1;
        i++;
        if (s[i] == '+' || s[i] == '-') {
            i++;
        }
        int expDigits = 0;
        while (isdigit((unsigned char)s[i])) {
            i++;
            expDigits++;
        }
        if (expDigits == 0) {
            return 0;
        }
    }

    if (isFloat) {
        /* optional float suffix f, F, l, L */
        if (s[i] == 'f' || s[i] == 'F' || s[i] == 'l' || s[i] == 'L') {
            i++;
        }
        return s[i] == '\0';
    }
    return isIntSuffix(s + i);
}

int isConstant(const char* str) {
    size_t len = strlen(str);
    if (len == 0) {
        return 0;
    }

    /* string or character literal: starts and ends with the same quote */
if (str[0] == '"') {
        return len >= 2 && str[len - 1] == '"';
    }
    if (str[0] == '\'') {
        return len >= 3 && str[len - 1] == '\'';   /* needs at least one character inside */
    }

    return isNumber(str);
}
/* ---------- decide the type of a finished lexeme ---------- */

void categorizeToken(Token* token) {
    const char* s = token->lexeme;

    if (s[0] == '#')                                    token->type = PREPROCESSOR;
    else if (isKeyword(s))                              token->type = KEYWORD;
    else if (isOperator(s))                             token->type = OPERATOR;
    else if (s[0] != '\0' && s[1] == '\0'
             && isSpecialCharacter(s[0]))               token->type = SPECIAL_CHARACTER;
    else if (isConstant(s))                             token->type = CONSTANT;
    else if (isIdentifier(s))                           token->type = IDENTIFIER;
    else                                                token->type = UNKNOWN;
}

/* ---------- reading characters ---------- */

/* skips whitespace, then looks at the next real character without using it up */
static int peekNonSpace(void) {
    int c;
    do {
        c = fgetc(fp);
    } while (c != EOF && isspace(c));
    if (c != EOF) {
        ungetc(c, fp);
    }
    return c;
}

/* skips whitespace and comments, returns the first real character */
static int skipIgnored(void) {
    int ch;
    while (1) {
        ch = fgetc(fp);
        if (ch == EOF) {
            return EOF;
        }
        if (isspace(ch)) {
            continue;
        }
        if (ch == '/') {
            int next = fgetc(fp);
            if (next == '/') {
                while ((ch = fgetc(fp)) != EOF && ch != '\n') {
                }
                continue;
            } else if (next == '*') {
                int prev = 0;
                while ((ch = fgetc(fp)) != EOF) {
                    if (prev == '*' && ch == '/') {
                        break;
                    }
                    prev = ch;
                }
                continue;
            } else {
                if (next != EOF) {
                    ungetc(next, fp);
                }
                return '/';
            }
        }
        return ch;
    }
}

/* Each reader below collects the characters of ONE token into t->lexeme.
   None of them decide the type. categorizeToken does that. */

static void readWord(int ch, Token* t) {
    int i = 0;
    while (ch != EOF && (isalnum(ch) || ch == '_')) {
        if (i < MAX_TOKEN_SIZE - 1) {
            t->lexeme[i++] = ch;
        }
        ch = fgetc(fp);
    }
    t->lexeme[i] = '\0';
    if (ch != EOF) {
        ungetc(ch, fp);
    }
}

/* is this '+' or '-' part of an exponent, as in 1e-5 ? (not for hex like 0x1E+2) */
static int isExponentSign(int ch, const char* lex, int i) {
    if (ch != '+' && ch != '-') {
        return 0;
    }
    if (i == 0 || (lex[i - 1] != 'e' && lex[i - 1] != 'E')) {
        return 0;
    }
    if (i >= 2 && lex[0] == '0' && (lex[1] == 'x' || lex[1] == 'X')) {
        return 0;
    }
    return 1;
}

static void readNumber(int ch, Token* t) {
    int i = 0;
    int seenDot = 0;
    while (ch != EOF && (isalnum(ch) || ch == '_'
                         || (ch == '.' && !seenDot)
                         || isExponentSign(ch, t->lexeme, i))) {
        if (ch == '.') {
            seenDot = 1;
        }
        if (i < MAX_TOKEN_SIZE - 1) {
            t->lexeme[i++] = ch;
        }
        ch = fgetc(fp);
    }
    t->lexeme[i] = '\0';
    if (ch != EOF) {
        ungetc(ch, fp);
    }
}
/* returns 1 if the closing quote was found, 0 otherwise */
static int readQuoted(int ch, Token* t) {
    int quote = ch;
    int i = 0;
    int closed = 0;
    t->lexeme[i++] = ch;

    while (1) {
        ch = fgetc(fp);
        if (ch == EOF || ch == '\n') {
            break;
        }
        if (i < MAX_TOKEN_SIZE - 1) {
            t->lexeme[i++] = ch;
        }
        if (ch == '\\') {
            ch = fgetc(fp);
            if (ch == EOF || ch == '\n') {
                break;
            }
            if (i < MAX_TOKEN_SIZE - 1) {
                t->lexeme[i++] = ch;
            }
        } else if (ch == quote) {
            closed = 1;
            break;
        }
    }
    t->lexeme[i] = '\0';
    if (ch == '\n') {
        ungetc(ch, fp);
    }
    return closed;
}
static void readOperator(int ch, Token* t) {
    t->lexeme[0] = ch;
    t->lexeme[1] = '\0';

    int next = fgetc(fp);
    if (next == EOF) {
        return;
    }
    t->lexeme[1] = next;
    t->lexeme[2] = '\0';
    if (!isOperator(t->lexeme)) {          /* not a pair: keep just one character */
        t->lexeme[1] = '\0';
        ungetc(next, fp);
        return;
    }

    /* it is a valid pair, so try a third character (for <<= and >>=) */
    int third = fgetc(fp);
    if (third == EOF) {
        return;
    }
    t->lexeme[2] = third;
    t->lexeme[3] = '\0';
    if (!isOperator(t->lexeme)) {
        t->lexeme[2] = '\0';
        ungetc(third, fp);
    }
}

static void readPreprocessor(int ch, Token* t) {
    int i = 0;
    while (ch != EOF && ch != '\n') {
        if (i < MAX_TOKEN_SIZE - 1) {
            t->lexeme[i++] = ch;
        }
        ch = fgetc(fp);
    }
    while (i > 0 && isspace((unsigned char)t->lexeme[i - 1])) {
        i--;
    }
    t->lexeme[i] = '\0';
}

/* ---------- the main function ---------- */

Token getNextToken() {
    Token t;
    t.lexeme[0] = '\0';

    int ch = skipIgnored();

    if (ch == EOF) {
        t.type = END_OF_FILE;
        return t;
    }

    /* 1. collect the lexeme, based on its first character */
    if (ch == '#') {
        readPreprocessor(ch, &t);
    } else if (isalpha(ch) || ch == '_') {
        readWord(ch, &t);
    } else if (isdigit(ch)) {
        readNumber(ch, &t);
    } else if (ch == '"' || ch == '\'') {
        if (!readQuoted(ch, &t)) {
            t.type = UNKNOWN;           /* missing closing quote */
            return t;
        }
    } else if (ch != '\0' && strchr(operators, ch) != NULL) {
        readOperator(ch, &t);
    } else {
        t.lexeme[0] = ch;               /* single character: special or unknown */
        t.lexeme[1] = '\0';
    }

    /* 2. decide its type */
    categorizeToken(&t);

    /* 3. a name directly followed by '[' is an array */
        if (t.type == IDENTIFIER && peekNonSpace() == '[') {
        t.type = ARRAY;
    }
    return t;
}