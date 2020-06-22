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

static Node *new_unary(NodeKind kind, Node *lhs) {
    Node *node = calloc(1, sizeof(Node));
    node->kind = kind;
    node->lhs = lhs;
    return node;
}

static Node *new_var_node(Var *var) {
    Node *node = new_node(ND_VAR);
    node->var = var;
    return node;
}

static Node *new_binary(NodeKind kind, Node *lhs, Node *rhs) {
    Node *node = new_node(kind);
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
// unary = ("+" unary | "-" unary)? primary
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
    if (consume("return")) {
        Node *node = new_unary(ND_RETURN, expr());
        expect(";");
        return node;
    }
    if (consume("if")) {
        Node *node = new_node(ND_IF);
        expect("(");
        node->cond = expr();
        expect(")");
        node->then = stmt();
        if (consume("else"))
            node->els = stmt();
        return node;
    }
    if (consume("while")) {
        Node *node = new_node(ND_WHILE);
        expect("(");
        node->cond = expr();
        expect(")");
        node->then = stmt();
        return node;
    }
    if (consume("for")) {
        Node *node = new_node(ND_FOR);
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
    if (consume("{")) {
        Node head = {};
        Node *cur = &head;
        while(!consume("}")) {
            cur->next = stmt();
            cur = cur->next;
        }
        Node *node = new_node(ND_BLOCK);
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

    Token *t = consume_ident();

    if (t) {
        if (consume("(")) {
            Node *node = new_node(ND_FUNCALL);
            node->funcname = strndup(t->str, t->len);
            node->args = func_args();
            return node;
        }
        
        Var *var = find_var(t);
        if(!var) 
            var = new_var(strndup(t->str, t->len));
        return new_var_node(var);
    }

    return new_num(expect_number());
}
