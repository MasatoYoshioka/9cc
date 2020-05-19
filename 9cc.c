#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <stdarg.h>
#include <stdbool.h>

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
};

Token *token;

void error(char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    fprintf(stderr, "\n");
    exit(1);
}

static long expect_number() {
    if (token->kind != TK_NUM)
        error("数値ではありません");
    long val = token->val;
    token = token->next;
    return val;
}

static Token *new_token(TokenKind kind, Token *cur, char *str) {
    Token *tok = calloc(1, sizeof(Token));
    tok->kind = kind;
    tok->str = str;
    cur->next = tok;
    return tok;
}

bool consume(char op) {
    if (token->kind != TK_RESERVED || token->str[0] != op)
        return false;
    token = token->next;
    return true;
}

static Token *tokenize(char *p) {
    Token head;
    head.next = NULL;
    Token *cur = &head;

    while (*p) {
        if (isspace(*p)) {
            p++;
            continue;
        }
        if (isdigit(*p)) {
            cur = new_token(TK_NUM, cur, p);
            cur->val = strtol(p, &p, 10);
            continue;
        }
        if (*p == '+' || *p == '-') {
            cur = new_token(TK_RESERVED, cur, p);
            cur->val = *p;
            p++;
            continue;
        }

        error("予期しない文字列です");
    }
    new_token(TK_EOF, cur, 0);
    return head.next;
}

int main(int argc, char **argv) {
    if (argc != 2) {
        error("引数の個数が正しくありません");
    }

    char *p = argv[1];
    token = tokenize(p);

    printf(".intel_syntax noprefix\n");
    printf(".global main\n");
    printf("main:\n");
    printf("  mov rax, %ld\n", expect_number());

    while(token->kind != TK_EOF) {
        if (consume('+')) {
            printf("  add rax, %ld\n", expect_number());
        }
        if (consume('-')) {
            printf("  sub rax, %ld\n", expect_number());
        }
    }
    printf("  ret\n");
    return 0;
}
