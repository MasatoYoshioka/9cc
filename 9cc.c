#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>

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

static long get_number(Token *tok) {
    if (tok->kind != TK_NUM) {
        exit(1);
    }
    return tok->val;
}

static Token *new_token(TokenKind kind, Token *cur, char *str) {
    Token *tok = calloc(1, sizeof(Token));
    tok->kind = kind;
    tok->str = str;
    cur->next = tok;
    return tok;
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

        fprintf(stderr, "予期しない文字です: '%c'\n", *p);
    }
    new_token(TK_EOF, cur, 0);
    return head.next;
}

int main(int argc, char **argv) {
    if (argc != 2) {
        fprintf(stderr, "%s: 引数の個数が正しくありません\n", argv[0]);
        return 1;
    }

    char *p = argv[1];
    Token *tok = tokenize(p);

    printf(".intel_syntax noprefix\n");
    printf(".global main\n");
    printf("main:\n");
    printf("  mov rax, %ld\n", get_number(tok));


    while(tok->kind != TK_EOF) {
        if (tok->kind == TK_RESERVED) {
            if (tok->val == '+') {
                printf("  add rax, %ld\n", get_number(tok->next));
            } else if (tok->val == '-') {
                printf("  sub rax, %ld\n", get_number(tok->next));
            }
        }
        tok = tok->next;
    }
    printf("  ret\n");
    return 0;
}
