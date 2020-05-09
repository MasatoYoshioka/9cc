#include <stdlib.h>
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
static Node *stmt() {
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
        return node;
    }
    return new_num(expect_number());
}

