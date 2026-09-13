#ifndef CMM_TOKEN_H
#define CMM_TOKEN_H

typedef enum TokenKind {
    TOK_EOF = 0,
    TOK_INT,
    TOK_FLOAT,
    TOK_ID,
    TOK_SEMI,
    TOK_COMMA,
    TOK_ASSIGNOP,
    TOK_RELOP,
    TOK_PLUS,
    TOK_MINUS,
    TOK_STAR,
    TOK_DIV,
    TOK_AND,
    TOK_OR,
    TOK_DOT,
    TOK_NOT,
    TOK_TYPE,
    TOK_LP,
    TOK_RP,
    TOK_LB,
    TOK_RB,
    TOK_LC,
    TOK_RC,
    TOK_STRUCT,
    TOK_RETURN,
    TOK_IF,
    TOK_ELSE,
    TOK_WHILE
} TokenKind;

const char *token_name(TokenKind kind);

extern int lexical_error_count;

#endif
