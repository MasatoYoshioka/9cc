#include "9cc.h"

static VarList *locals;

static Var *find_var(Token *tok) {
    for (VarList *vl = locals; vl; vl = vl->next) {
        Var *var = vl->var;
        if (strlen(var->name) == tok->len 
                && !memcmp(tok->str, var->name, tok->len))
            return var;
    }
    return NULL;
}

static Var *new_var(char *name) {
    Var *var = calloc(1, sizeof(Var));
    var->name = name;

    VarList *vl = calloc(1, sizeof(VarList));
    vl->var = var;
    vl->next = locals;
    locals = vl;
    return var;
}

static VarList *read_func_params() {
    if (consume(")"))
        return NULL;

    VarList *head = calloc(1, sizeof(VarList));
    head->var = new_var(expect_ident());
    VarList *cur = head;

    while(!consume(")")) {
        expect(",");
        cur->next = calloc(1, sizeof(VarList));
        cur->next->var = new_var(expect_ident());
        cur = cur->next;
    }
    return head;
}

static Node *new_node(NodeKind kind, Token *tok)
{
    Node *node = calloc(1, sizeof(Node));
    node->kind = kind;
    node->tok = tok;
    return node;
}

static Node *new_num(long num, Token *tok) {
    Node *node = new_node(ND_NUM, tok);
    node->val = num;
    return node;
}

static Node *new_unary(NodeKind kind, Node *lhs, Token *tok) {
    Node *node = new_node(kind, tok);
    node->kind = kind;
    node->lhs = lhs;
    return node;
}

static Node *new_var_node(Var *var, Token *tok) {
    Node *node = new_node(ND_VAR, tok);
    node->var = var;
    return node;
}

static Node *new_binary(NodeKind kind, Node *lhs, Node *rhs, Token *tok) {
    Node *node = new_node(kind, tok);
    node->kind = kind;
    node->lhs = lhs;
    node->rhs = rhs;
    return node;
}

static Function *function();
static Node *stmt();
static Node *expr();
static Node *assign();
static Node *equality();
static Node *relational();
static Node *add();
static Node *mul();
static Node *unary();
static Node *primary();

// program = function*
// function = ident "(" ")" "{" stmt "}"
// stmt = expr ";"
//         | "{" stmt* "}"
//         | "if" "(" expr ")" stmt ("else" stmt)?
//         | "while" "(" expr ")" stmt
//         | "for" "(" expr? ";" expr? ";" expr? ")" stmt
//         | "return" expr ";"
// expr = assign
// assign = equality ("=" assign)?
// equality = relational ("==" relational| "!=" relational)*
// relational = add ("<=" add | "<" add | ">=" add | ">" add)*
// add = mul ("+" mul | "-" mul)*
// mul = unary ("*" unary | "/" unary)*
// unary = ("+" | "-" | "*" | "&" )? unary
// primary = num | indent func_args? | "(" expr ")"

// program = function*
Function *program() {
    Function head = {};
    Function *cur = &head;

    while(!at_eof()) {
        cur->next = function();
        cur = cur->next;
    }
    return head.next;
}

// function = ident "(" params? ")" "{" stmt "}"
// params = ident ("," ident)*
static Function *function() {
    locals = NULL;

    Function *fn = calloc(1, sizeof(Function));
    fn->name = expect_ident();
    expect("(");
    fn->params = read_func_params();
    expect("{");

    Node head = {};
    Node *cur = &head;

    while(!consume("}")) {
        cur->next = stmt();
        cur = cur->next;
    }

    fn->node = head.next;
    fn->locals = locals;
    return fn;
}

// stmt = expr ";"
//         | "{" stmt* "}"
//         | "if" "(" expr ")" stmt ("else" stmt)?
//         | "while" "(" expr ")" stmt
//         | "for" "(" expr? ";" expr? ";" expr? ")" stmt
//         | "return" expr ";"
static Node *stmt() {
    Token *tok;
    if ((tok = consume("return"))) {
        Node *node = new_unary(ND_RETURN, expr(), tok);
        expect(";");
        return node;
    }
    if ((tok = consume("if"))) {
        Node *node = new_node(ND_IF, tok);
        expect("(");
        node->cond = expr();
        expect(")");
        node->then = stmt();
        if (consume("else"))
            node->els = stmt();
        return node;
    }
    if ((tok = consume("while"))) {
        Node *node = new_node(ND_WHILE, tok);
        expect("(");
        node->cond = expr();
        expect(")");
        node->then = stmt();
        return node;
    }
    if ((tok = consume("for"))) {
        Node *node = new_node(ND_FOR, tok);
        expect("(");
        if (!consume(";")) {
            node->init = expr();
            expect(";");
        }
        if (!consume(";")) {
            node->cond = expr();
            expect(";");
        }
        if (!consume(")")) {
            node->inc = expr();
            expect(")");
        }
        node->then = stmt();
        return node;
    }
    if ((tok = consume("{"))) {
        Node head = {};
        Node *cur = &head;
        while(!consume("}")) {
            cur->next = stmt();
            cur = cur->next;
        }
        Node *node = new_node(ND_BLOCK, tok);
        node->body = head.next;
        return node;
    }
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

    Token *tok;
    if ((tok = consume("=")))
        node = new_binary(ND_ASSIGN, node, assign(), tok);
    return node;
}

// equality = relational ("==" relational| "!=! relational)*
static Node *equality() {
    Node *node = relational();
    Token *tok;

    for (;;) {
        if ((tok = consume("=="))) {
            node = new_binary(ND_EQ, node, relational(), tok);
            continue;
        }
        if ((tok = consume("!="))) {
            node = new_binary(ND_NE, node, relational(), tok);
            continue;
        }
        break;
    }
    return node;
}

// relational = add ("<=" add | "<" add | ">=" add | ">" add)*
static Node *relational() {
    Node *node = add();
    Token *tok;

    for (;;) {
        if ((tok = consume("<="))) {
            node = new_binary(ND_LE, node, add(), tok);
            continue;
        }
        if ((tok = consume("<"))) {
            node = new_binary(ND_LT, node, add(), tok);
            continue;
        }
        if ((tok = consume(">="))) {
            node = new_binary(ND_LE, add(), node, tok);
            continue;
        }
        if ((tok = consume(">"))) {
            node = new_binary(ND_LT, add(), node, tok);
            continue;
        }
        break;
    }
    return node;
}

static Node *add() {
    Node *node = mul();
    Token *tok;

    for(;;) {
        if ((tok = consume("+"))) {
            node = new_binary(ND_ADD, node, mul(), tok);
            continue;
        }
        if ((tok = consume("-"))) {
            node = new_binary(ND_SUB, node, mul(), tok);
            continue;
        }
        break;
    }
    return node;
}

// mul = unary ("*" unary | "/" unary)*
static Node *mul() {
    Node *node = unary();
    Token *tok;

    for(;;) {
        if ((tok = consume("*"))) {
            node = new_binary(ND_MUL, node, unary(), tok);
            continue;
        }
        if ((tok = consume("/"))) {
            node = new_binary(ND_DIV, node, unary(), tok);
            continue;
        }
        break;
    }
    return node;
}

// unary = ("+" | "-" | "*" | "&" )? unary
static Node *unary() {
    Token *tok;
    if (consume("+"))
        return unary();
    if ((tok = consume("-")))
        return new_binary(ND_SUB, new_num(0, tok), unary(), tok);
    if ((tok = consume("&")))
        return new_unary(ND_ADDR, unary(), tok);
    if ((tok = consume("*")))
        return new_unary(ND_DEREF, unary(), tok);
    return primary();
}

// func_args = "(" (assign ("," assign)*)? ")"
static Node *func_args() {
    if (consume(")"))
        return NULL;

    Node *head = assign();
    Node *cur = head;
    while (consume(",")) {
        cur->next = assign();
        cur = cur->next;
    }
    expect(")");
    return head;
}

// primary = num 
//          | ident func_args?
//          | "(" expr ")"
static Node *primary() {
    if (consume("(")) {
        Node *node = expr();
        expect(")");
        return node;
    }

    Token *tok;

    if ((tok = consume_ident())) {
        if (consume("(")) {
            Node *node = new_node(ND_FUNCALL, tok);
            node->funcname = strndup(tok->str, tok->len);
            node->args = func_args();
            return node;
        }
        
        Var *var = find_var(tok);
        if(!var) 
            var = new_var(strndup(tok->str, tok->len));
        return new_var_node(var, tok);
    }

    tok = token;
    if (tok->kind != TK_NUM) {
        error_tok(tok, "式ではありません");
    }

    return new_num(expect_number(), tok);
}
