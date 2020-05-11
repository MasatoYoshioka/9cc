#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include "9cc.h"

static Node *stmt();
static Node *expr();
static Node *assign();
static Node *equality();
static Node *relational();
static Node *add();
static Node *mul();
static Node *unary();
static Node *primary();

LVar *locals;

static LVar *find_lvar(Token *tok) {
    for (LVar *var = locals; var; var = var->next)
        if (var->len == tok->len && !memcmp(tok->str, var->name, var->len))
            return var;
    return NULL;
}

// エラーを報告するための関数
// printfと同じ引数を取る
void error(char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    fprintf(stderr, "\n");
    exit(1);
}


// 次のトークンが期待している記号の時には、トークンを1つ読み進めんて
// 真を返す。それ以外の場合は偽を返す
bool consume(char *op) {
    if (token->kind != TK_RESERVED ||
      strlen(op) != token->len ||
      memcmp(token->str, op, token->len))
        return false;
    token = token->next;
    return true;
}

Token *consume_ident() {
    if (token->kind != TK_IDENT)
        return NULL;
    Token *t = token;
    token = token->next;
    return t;
}

// 次のトークンが期待している記号の時には、トークンを1つ進める。
// それ以外の場合にはエラーを報告する
void expect(char *op) {
    if (token->kind != TK_RESERVED || strlen(op) != token->len ||
      memcmp(token->str, op, token->len))
        error_at(token->str, "'%c'ではありません", op);
    token = token->next;
}

// 次のトークンが数値の場合、トークンを1つ読み進めてその数値を返す。
// それ以外の場合にはエラーを報告する
int expect_number() {
    if (token->kind != TK_NUM)
        error_at(token->str, "整数ではありません");
    int val = token->val;
    token = token->next;
    return val;
}

bool at_eof() {
    return token->kind == TK_EOF;
}


Node *new_node(NodeKind kind) {
    Node *node = calloc(1, sizeof(Node));
    node->kind = kind;
    return node;
}
Node *new_binary(NodeKind kind, Node *lhs, Node *rhs) {
    Node *node = calloc(1, sizeof(Node));
    node->kind = kind;
    node->lhs = lhs;
    node->rhs = rhs;
    return node;
}

Node *new_num(int val) {
    Node *node = calloc(1, sizeof(Node));
    node->kind = ND_NUM;
    node->val = val;
    return node;
}

LVar *new_lvar(Token *tok) {
    LVar *lvar = calloc(1, sizeof(LVar));
    lvar->next = locals;
    lvar->name = tok->str;
    lvar->len = tok->len;;
    return lvar;
}

Node *program() {
    Node head = {};
    Node *cur = &head;

    while (!at_eof()) {
        cur->next = stmt();
        cur = cur->next;
    }
    return head.next;
}

// stmt = expr ";"
//      | "return" expr ";"
static Node *stmt() {

    if (consume("return")) {
        Node *node = new_node(ND_RETURN);
        node->lhs = expr();
        expect(";");
        return node;
    }
    Node *node = expr();
    expect(";");
    return node;
}

// expr = assign
static Node *expr() {
    return assign();
}

// equality("=" assign)?
static Node *assign() {
    Node *node = equality();

    if (consume("="))
        node = new_binary(ND_ASSIGN, node, assign());
    return node;
}

// equality = relational("==" relational | "!=" relational)*
static Node *equality() {
    Node *node = relational();

    for (;;) {
        if (consume("==")) 
            node = new_binary(ND_EQ, node, relational());
        else if (consume("!=")) 
            node = new_binary(ND_NE, node, relational());
        else
            return node;
    }
}

// relational = add("<=" add | ">=" add | "<" add | ">" add)*
static Node *relational() {
    Node *node = add();

    for (;;) {
        if (consume("<"))
            node = new_binary(ND_LT, node, add());
        else if (consume("<="))
            node = new_binary(ND_LE, node, add());
        else if (consume(">")) 
            node = new_binary(ND_LT, add(), node);
        else if (consume(">="))
            node = new_binary(ND_LE, add(), node);
        else
            return node;
    }
}

// add = mul("+" mul | "-" mul)*
static Node *add() {
    Node *node = mul();
    
    for(;;) {
        if (consume("+"))
            node = new_binary(ND_ADD, node, mul());
        else if (consume("-"))
            node = new_binary(ND_SUB, node, mul());
        else
            return node;
    }
}

// mul = unary("*" unary | "/" unary)*
static Node *mul() {
    Node *node = unary();

    for(;;) {
        if (consume("*")) 
            node = new_binary(ND_MUL, node, unary());
        else if (consume("/"))
            node = new_binary(ND_DIV, node, unary());
        else
            return node;
    }
}

// unary = ("+" | "-")? unary
//       | primary
static Node *unary() {
    if (consume("+")) {
        return unary();
    }
    if (consume("-")) {
        return new_binary(ND_SUB, new_num(0), unary());
    }
    return primary();
}

// primary = "(" expr ")" | num
static Node *primary() {
    if (consume("(")) {
        Node *node = expr();
        expect(")");
        return node;
    }

    Token *t = consume_ident();
    if (t) {
        Node *node = calloc(1, sizeof(Node));
        node->kind = ND_LVAR;
        node->offset = (t->str[0] - 'a' + 1) * 8;

        LVar *lvar = find_lvar(t);
        if (!lvar) {
            LVar *lvar = new_lvar(t);
            locals = lvar;
            lvar->offset = locals->offset + 8;
        }
        return node;
    }
    return new_num(expect_number());
}

