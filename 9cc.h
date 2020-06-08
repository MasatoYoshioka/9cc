#define _GNU_SOURCE
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <stdarg.h>

// tokenize.c
typedef enum {
    TK_RESERVED, // punctuators
    TK_INDENT, // 識別子
    TK_NUM, // number
    TK_EOF, // end of file marker
} TokenKind;

typedef struct Token Token;

struct Token {
    TokenKind kind;
    Token *next;
    int val;
    char *str;
    int len;
};

void error(char *fmt, ...);
void error_at(char *loc, char *fmt, ...);

Token *tokenize(char *input);
bool consume(char *op);
Token *consume_ident();
long expect_number();
void expect(char *op);
bool at_eof();

extern Token *token;

typedef struct Var Var;

struct Var {
    Var *next;
    char *name;
    int offset;
};

// parser
typedef enum {
    ND_ADD,
    ND_SUB,
    ND_MUL,
    ND_DIV,
    ND_NUM,
    ND_EQ, // ==
    ND_NE, // !=
    ND_LT, // <
    ND_LE, // <=
    ND_ASSIGN, // =
    ND_VAR, // Variable
} NodeKind;

// Ast Node
typedef struct Node Node;

struct Node {
    NodeKind kind; //種別
    Node *lhs; // 左辺
    Node *rhs; // 右辺
    Node *next; // 次のNode
    Var *var; // ND_VARの時に使う 変数の名前
    long val;  // 値
};

typedef struct Function Function;

struct Function {
    Node *node;
    Var *locals;
    int stack_size;
};

Function *program();

// codegen
void codegen(Function *prog);
