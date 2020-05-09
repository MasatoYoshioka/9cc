#include <stdbool.h>
//
// Tokenizer
//
// トークンの種類
typedef enum {
    TK_RESERVED, // 記号
    TK_NUM,      // 整数トークン
    TK_IDENT,    // 識別子
    TK_EOF,      // 入力の終わりを表すトークン
} TokenKind;

typedef struct Token Token;

// トークン型
struct Token {
    TokenKind kind;  // トークンの型
    Token     *next; // 次の入力トークン
    int       val;   // kindがTK_NUMの場合、その数値
    char      *str;  // トークン文字列
    int       len;   // トークンの長さ
};

void error(char *fmt, ...);
void error_at(char *loc, char *fmt, ...);
bool consume(char *op);
Token *consume_ident();
void expect(char *go);
int expect_number();
bool at_eof();
Token *tokenize();

char *user_input;
// 現在着目しているトークン
Token *token;

// 
// Parser
// 
typedef enum {
    ND_ADD,    // +
    ND_SUB,    // -
    ND_MUL,    // *
    ND_DIV,    // /
    ND_NUM,    // 整数
    ND_EQ,     // ==
    ND_NE,     // !=
    ND_LT,     // <
    ND_LE,     // <=
    ND_ASSIGN, // =
    ND_LVAR,    // Local Variable
} NodeKind;

typedef struct Node Node;

struct Node {
    NodeKind kind; // ノードの型
    Node     *next; // 次のノード
    Node     *lhs; // 左辺
    Node     *rhs; // 右辺
    int      val; // kindがND_NUMの時に使う
    int     offset; // kindがNO_VARの時に使う
};

void codegen(Node *node);

Node *program(void);
