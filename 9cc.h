#include <stdbool.h>
// tokenize.c
typedef enum {
    TK_RESERVED, // punctuators
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
long expect_number();
void expect(char *op);

extern Token *token;

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
} NodeKind;

// Ast Node
typedef struct Node Node;

struct Node {
    NodeKind kind; //種別
    Node *lhs; // 左辺
    Node *rhs; // 右辺
    long val;  // 値
};

Node *expr();

// codegen
void gen(Node *node);
