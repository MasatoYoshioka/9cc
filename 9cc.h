#define _GNU_SOURCE
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <stdarg.h>

typedef struct Type Type;

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
void error_tok(Token *tok, char *fmt, ...);

Token *peek(char *s);
Token *tokenize(char *input);
Token *consume(char *op);
Token *consume_ident();
long expect_number();
void expect(char *op);
bool at_eof();
char *expect_ident();

extern Token *token;

typedef struct Var Var;

struct Var {
    char *name;
    Type *ty;
    bool is_local;
    int offset;
};

typedef struct VarList VarList;

struct VarList {
    VarList *next;
    Var *var;
};

// parser
typedef enum {
    ND_ADD, // num + num
    ND_PTR_ADD, // ptr + num or num + ptr
    ND_SUB, // num - num
    ND_PTR_SUB, // ptr - num 
    ND_PTR_DIFF, // ptr - ptr
    ND_MUL,
    ND_DIV,
    ND_NUM,
    ND_EQ, // ==
    ND_NE, // !=
    ND_LT, // <
    ND_LE, // <=
    ND_ASSIGN, // =
    ND_VAR, // Variable
    ND_RETURN, // Return
    ND_IF, // IF
    ND_WHILE, // WHILE
    ND_FOR, // FOR
    ND_BLOCK, // Block {}
    ND_FUNCALL, // Function cal
    ND_EXPR_STMT, // Expression statement
    ND_ADDR, // *
    ND_DEREF, // &
    ND_NULL,  // empty statement
} NodeKind;

// Ast Node
typedef struct Node Node;

struct Node {
    NodeKind kind; //種別
    Node *lhs; // 左辺
    Node *rhs; // 右辺
    Node *next; // 次のNode
    Token *tok; // Token
    Var *var; // ND_VARの時に使う 変数の名前
    long val;  // 値
    Node *cond; // if cond
    Node *then; // if then
    Node *els; // if else
    Node *init; // for init
    Node *inc; // for increment
    Node *body; // {} Block
    char *funcname; // funcion call name
    Node *args; // function args
    Type *ty; // type, e.g. int or pointer to int
};

typedef struct Function Function;

struct Function {
    Function *next;
    char *name;
    VarList *params;
    Node *node;
    VarList *locals;
    int stack_size;
};

typedef struct {
    VarList *globals;
    Function *fns;

} Program;

Program *program();

typedef enum {
    TY_INT,
    TY_PTR,
    TY_ARRAY,
} TypeKind;

struct Type {
    TypeKind kind;
    int size; // sizeof() value
    Type *base; // pointer
    int array_len;
};

extern Type *int_type;

bool is_integer(Type *ty);
Type *pointer_to(Type *base);
Type *array_of(Type *base, int size);
void add_type(Node *node);

// codegen
void codegen(Program *prog);
