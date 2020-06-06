#include <stdlib.h>
#include "9cc.h"

static Var *new_var(char name) {
    Var *var = calloc(1, sizeof(Var));
    var->name = name;
    var->offset = (name - 'a' + 1) * 8;
    return var;
}

static Node *new_node(NodeKind kind)
{
    Node *node = calloc(1, sizeof(Node));
    node->kind = kind;
    return node;
}

static Node *new_num(long num) {
    Node *node = new_node(ND_NUM);
    node->val = num;
    return node;
}

static Node *new_var_node(char name) {
    Node *node = new_node(ND_VAR);
    node->var = new_var(name);
    return node;
}

static Node *new_binary(NodeKind kind, Node *lhs, Node *rhs) {
    Node *node = new_node(kind);
    node->kind = kind;
    node->lhs = lhs;
    node->rhs = rhs;
    return node;
}

static Node *stmt();
static Node *expr();
static Node *assign();
static Node *equality();
static Node *relational();
static Node *add();
static Node *mul();
static Node *unary();
static Node *primary();

// program = stmt
// stmt = expr ";"
// expr = assign
// assign = equality ("=" assign)?
// equality = relational ("==" relational| "!=" relational)*
// relational = add ("<=" add | "<" add | ">=" add | ">" add)*
// add = mul ("+" mul | "-" mul)*
// mul = unary ("*" unary | "/" unary)*
// unary = ("+" unary | "-" unary)? primary
// primary = num | indent | "(" expr ")"

Node *program() {
    Node head = {};
    Node *cur = &head;

    while(!at_eof()) {
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

// assign
static Node *expr() {
    return assign();
}

// equality ("=" assign)?
static Node *assign() {
    Node *node = equality();

    if (consume("="))
        node = new_binary(ND_ASSIGN, node, assign());
    return node;
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

// primary = num | ident | "(" expr ")"
static Node *primary() {
    if (consume("(")) {
        Node *node = expr();
        expect(")");
        return node;
    }

    Token *t = consume_ident();

    if (t) {
        return new_var_node(*t->str);
    }

    return new_num(expect_number());
}
