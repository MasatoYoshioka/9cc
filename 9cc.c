#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <stdarg.h>
#include <stdbool.h>
#include <string.h>
#include "9cc.h"

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

static Node *new_num(long num) {
    Node *node = calloc(1, sizeof(Node));
    node->kind = ND_NUM;
    node->val = num;
    return node;
}

static Node *new_binary(NodeKind kind, Node *lhs, Node *rhs) {
    Node *node = calloc(1, sizeof(Node));
    node->kind = kind;
    node->lhs = lhs;
    node->rhs = rhs;
    return node;
}

static Node *equality();
static Node *relational();
static Node *add();
static Node *mul();
static Node *unary();
static Node *primary();

// expr = add
// equality = relational ("==" relational| "!=" relational)*
// relational = add ("<=" add | "<" add | ">=" add | ">" add)*
// add = mul ("+" mul | "-" mul)*
// mul = unary ("*" unary | "/" unary)*
// unary = ("+" unary | "-" unary)? primary
// primary = num | "(" expr ")"
static Node *expr() {
    return equality();
}

// equality = relational ("==" relational| "!=! relational)*
static Node *equality() {
    Node *node = relational();

    for (;;) {
        if (consume("==")) {
            node = new_binary(ND_EQ, node, relational());
            continue;
        }
        if (consume("!=")) {
            node = new_binary(ND_NE, node, relational());
            continue;
        }
        break;
    }
    return node;
}

// relational = add ("<=" add | "<" add | ">=" add | ">" add)*
static Node *relational() {
    Node *node = add();

    for (;;) {
        if (consume("<=")) {
            node = new_binary(ND_LE, node, add());
            continue;
        }
        if (consume("<")) {
            node = new_binary(ND_LT, node, add());
            continue;
        }
        if (consume(">=")) {
            node = new_binary(ND_LE, add(), node);
            continue;
        }
        if (consume(">")) {
            node = new_binary(ND_LT, add(), node);
            continue;
        }
        break;
    }
    return node;
}

static Node *add() {
    Node *node = mul();

    for(;;) {
        if (consume("+")) {
            node = new_binary(ND_ADD, node, mul());
            continue;
        }
        if (consume("-")) {
            node = new_binary(ND_SUB, node, mul());
            continue;
        }
        break;
    }
    return node;
}

// mul = unary ("*" unary | "/" unary)*
static Node *mul() {
    Node *node = unary();

    for(;;) {
        if (consume("*")) {
            node = new_binary(ND_MUL, node, unary());
            continue;
        }
        if (consume("/")) {
            node = new_binary(ND_DIV, node, unary());
            continue;
        }
        break;
    }
    return node;
}

// unary = ("+" unary | "-" unary)? primary
static Node *unary() {
    if (consume("+"))
        return unary();
    if (consume("-"))
        return new_binary(ND_SUB, new_num(0), unary());
    return primary();
}

// primary = num | "(" expr ")"
static Node *primary() {
    if (consume("(")) {
        Node *node = expr();
        expect(")");
        return node;
    }
    return new_num(expect_number());
}


static void gen(Node *node) {
    if (node->kind == ND_NUM) {
        printf("  push %ld\n", node->val);
        return;
    }

    gen(node->lhs);
    gen(node->rhs);

    printf("  pop rdi\n");
    printf("  pop rax\n");

    switch(node->kind) {
        case ND_ADD:
            printf("  add rax, rdi\n");
            break;
        case ND_SUB:
            printf("  sub rax, rdi\n");
            break;
        case ND_MUL:
            printf("  imul rax, rdi\n");
            break;
        case ND_DIV:
            printf("  cqo\n");
            printf("  idiv rdi\n");
            break;
        case ND_EQ:
            printf("  cmp rax, rdi\n");
            printf("  sete al\n");
            printf("  movzb rax, al\n");
            break;
        case ND_NE:
            printf("  cmp rax, rdi\n");
            printf("  setne al\n");
            printf("  movzb rax, al\n");
            break;
        case ND_LE:
            printf("  cmp rax, rdi\n");
            printf("  setle al\n");
            printf("  movzb rax, al\n");
            break;
        case ND_LT:
            printf("  cmp rax, rdi\n");
            printf("  setl al\n");
            printf("  movzb rax, al\n");
            break;

    }
    printf("  push rax\n");
}

int main(int argc, char **argv) {
    if (argc != 2) {
        error("引数の個数が正しくありません");
    }

    token = tokenize(argv[1]);
    Node *node = expr();

    printf(".intel_syntax noprefix\n");
    printf(".global main\n");
    printf("main:\n");

    gen(node);

    printf("  pop rax\n");
    printf("  ret\n");
    return 0;
}
