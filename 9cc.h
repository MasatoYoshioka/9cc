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
