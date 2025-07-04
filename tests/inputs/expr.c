#include "lexer.h"
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include <stdbool.h>

StringTokenKindMap operators[] = {
    {"==", EQ}, {"!=", NEQ}, {"<=", LTE}, {">=", GTE}, {"&&", AND}, {"||", OR},
    {"<<", SHL}, {">>", SHR}, {"++", INC}, {"--", DEC}, {"*", POINTER}, {"->", MEMBER},
    {"+", ADD}, {"-", SUB}, {"*", MUL}, {"/", DIV}, {"%", MOD}, {"=", ASSIGN},
    {"(", L_PARENTHESES}, {")", R_PARENTHESES}, {";", SEMICOLON}, {",", COMMA},
    {"{", L_BRACE}, {"}", R_BRACE}, {"[", L_BRACKET}, {"]", R_BRACKET},
    {"<", LT}, {">", GT}, {".", DOT}, {"!", NOT}, {"?", QUESTION}, {":", COLON},
    {"|", BITOR}, {"^", BITXOR}, {"~", BITNOT}, {"#", HASH}, {"&", AMPERSAND}
};

StringTokenKindMap reservedWords[] = {
    {"sizeof", SIZEOF}, {"bool", BOOL}, {"int", INT}, {"char", CHAR}, {"float", FLOAT},
    {"double", DOUBLE}, {"void", VOID}, {"long", LONG}, {"short", SHORT},
    {"unsigned", UNSIGNED}, {"signed", SIGNED}, {"const", CONST}, {"static", STATIC},
    {"extern", EXTERN}, {"auto", AUTO}, {"register", REGISTER},
    {"if", IF}, {"else", ELSE}, {"while", WHILE}, {"do", DO}, {"for", FOR},
    {"switch", SWITCH}, {"case", CASE}, {"default", DEFAULT},
    {"break", BREAK}, {"continue", CONTINUE}, {"return", RETURN},
    {"typedef", TYPEDEF}, {"struct", STRUCT}, {"union", UNION}, {"enum", ENUM}
};

Token *createToken(Token *cur, int kind, char *value) {
    Token *newTk = malloc(sizeof(Token));
    newTk->kind = kind;
    newTk->value = strdup(value);
    newTk->next = NULL;
    cur->next = newTk;
    return newTk;
}

bool isComment(char *ptr, char *buffer) {
    if (ptr[0] != '/' || ptr[1] != '/') return false;
    while (*ptr && *ptr != '\n') *buffer++ = *ptr++;
    *buffer = '\0';
    return true;
}

bool isCommentBlock(char *ptr, char *buffer) {
    if (ptr[0] != '/' || ptr[1] != '*') return false;
    ptr += 2;
    while (*ptr && !(ptr[0] == '*' && ptr[1] == '/')) *buffer++ = *ptr++;
    if (*ptr) ptr += 2;
    *buffer = '\0';
    return true;
}

bool isOperator(char *ptr, TokenKind *tk, char *buffer) {
    for (int i = 0; i < sizeof(operators)/sizeof(operators[0]); i++) {
        size_t len = strlen(operators[i].str);
        if (strncmp(ptr, operators[i].str, len) == 0) {
            strcpy(buffer, operators[i].str);
            if (tk) *tk = operators[i].kind;
            return true;
        }
    }
    return false;
}

bool isReservedWord(char *ptr, TokenKind *tk, char *buffer) {
    for (int i = 0; i < sizeof(reservedWords)/sizeof(reservedWords[0]); i++) {
        size_t len = strlen(reservedWords[i].str);
        if (strncmp(ptr, reservedWords[i].str, len) == 0 &&
            !isalnum(ptr[len]) && ptr[len] != '_') {
            strcpy(buffer, reservedWords[i].str);
            if (tk) *tk = reservedWords[i].kind;
            return true;
        }
    }
    return false;
}

bool isNumber(char *ptr, char *buffer) {
    char *start = ptr;
    if (*ptr == '+' || *ptr == '-') ptr++;
    char *numStart = ptr;
    while (isdigit(*ptr)) ptr++;
    if (*ptr == '.') {
        ptr++;
        while (isdigit(*ptr)) ptr++;
    }
    if (ptr == numStart) return false;
    size_t len = ptr - start;
    strncpy(buffer, start, len);
    buffer[len] = '\0';
    return true;
}

bool isIdentifier(char *ptr, char *buffer) {
    if (!isalpha(*ptr) && *ptr != '_') return false;
    char *start = ptr;
    while (isalnum(*ptr) || *ptr == '_') ptr++;
    size_t len = ptr - start;
    strncpy(buffer, start, len);
    buffer[len] = '\0';
    return true;
}

bool isStringLiteral(char *ptr, char *buffer) {
    if (*ptr != '"') return false;
    char *start = ptr++;
    while (*ptr && (*ptr != '"' || *(ptr - 1) == '\\')) ptr++;
    if (*ptr != '"') return false;
    ptr++;
    size_t len = ptr - start;
    strncpy(buffer, start, len);
    buffer[len] = '\0';
    return true;
}

bool isCharLiteral(char *ptr, char *buffer) {
    if (*ptr != '\'') return false;
    char *start = ptr++;
    if (*ptr == '\\') ptr++;
    if (*ptr) ptr++;
    if (*ptr != '\'') return false;
    ptr++;
    size_t len = ptr - start;
    strncpy(buffer, start, len);
    buffer[len] = '\0';
    return true;
}


Token *lexer(char *input) {
    Token head = {0};
    Token *cur = &head;
    char *ptr = input;
    char buffer[256];
    TokenKind kind;

    while (*ptr) {
        buffer[0] = '\0';

        if (isspace(*ptr)) { ptr++; continue; }

        if (isComment(ptr, buffer)) {
            ptr += strlen(buffer);
            while (*ptr && *ptr != '\n') ptr++;
            if (*ptr == '\n') ptr++;
            continue;
        }

        if (isCommentBlock(ptr, buffer)) {
            ptr += strlen(buffer) + 4;
            continue;
        }

        if (isOperator(ptr, &kind, buffer)) {
            cur = createToken(cur, kind, buffer);
            ptr += strlen(buffer);
            continue;
        }

        if (isReservedWord(ptr, &kind, buffer)) {
            cur = createToken(cur, kind, buffer);
            ptr += strlen(buffer);
            continue;
        }

        if (isNumber(ptr, buffer)) {
            cur = createToken(cur, NUMBER, buffer);
            ptr += strlen(buffer);
            continue;
        }

        if (isIdentifier(ptr, buffer)) {
            cur = createToken(cur, IDENTIFIER, buffer);
            ptr += strlen(buffer);
            continue;
        }

        if (isStringLiteral(ptr, buffer)) {
            cur = createToken(cur, STRING_LITERAL, buffer);
            ptr += strlen(buffer);
            continue;
        }

        if (isCharLiteral(ptr, buffer)) {
            cur = createToken(cur, CHAR_LITERAL, buffer);
            ptr += strlen(buffer);
            continue;
        }
        

        ptr++;
    }

    createToken(cur, EOT, "");
    return head.next;
}
