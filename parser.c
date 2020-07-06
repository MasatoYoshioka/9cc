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

static Var *new_var(char *name, Type *ty) {
    Var *var = calloc(1, sizeof(Var));
    var->name = name;
    var->ty = ty;

    VarList *vl = calloc(1, sizeof(VarList));
    vl->var = var;
    vl->next = locals;
    locals = vl;
    return var;
}

static Type *basetype() {
    expect("int");
    Type *ty = int_type;
    while (consume("*"))
        ty = pointer_to(ty);
    return ty;
}

static Type *read_type_suffix(Type *base){
    if (!consume("["))
        return base;
    int sz = expect_number();
    expect("]");
    base = read_type_suffix(base);
    return array_of(base, sz);
}

static VarList *read_func_param() {
    Type *ty = basetype();
    char *name = expect_ident();
    ty = read_type_suffix(ty);


    VarList *vl = calloc(1, sizeof(VarList));
    vl->var = new_var(name, ty);
    return vl;
}

static VarList *read_func_params() {
    if (consume(")"))
        return NULL;

    VarList *head = read_func_param();
    VarList *cur = head;

    while(!consume(")")) {
        expect(",");
        cur->next = read_func_param();
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
static Node *stmt2();
static Node *expr();
static Node *assign();
static Node *equality();
static Node *relational();
static Node *add();
static Node *mul();
static Node *unary();
static Node *postfix();
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
//       | postfix   
// postfix = primary ("[" expr "]")*
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

// function = basetype ident "(" params? ")" "{" stmt "}"
// params = ident ("," param)*
// param  = basetype ident
static Function *function() {
    locals = NULL;

    Function *fn = calloc(1, sizeof(Function));
    basetype();
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

// declation = basetype ident ("[" num "]")* ("=" expr) ";"
static Node *declaration() {
    Token *tok = token;
    Type *ty = basetype();
    char *name = expect_ident();
    ty = read_type_suffix(ty);
    Var *var = new_var(name, ty);
    if (consume(";"))
        return new_node(ND_NULL, tok);

    expect("=");
    Node *lhs = new_var_node(var, tok);
    Node *rhs = expr();
    expect(";");
    Node *node = new_binary(ND_ASSIGN, lhs, rhs, tok);
    return new_unary(ND_EXPR_STMT, node, tok);
}

static Node *read_expr_stmt() {
    Token *tok = token;
    return new_unary(ND_EXPR_STMT, expr(), tok);
}

static Node *stmt() {
    Node *node = stmt2();
    add_type(node);
    return node;
}

// stmt = expr ";"
//         | "{" stmt* "}"
//         | "if" "(" expr ")" stmt ("else" stmt)?
//         | "while" "(" expr ")" stmt
//         | "for" "(" expr? ";" expr? ";" expr? ")" stmt
//         | declaration
//         | "return" expr ";"
static Node *stmt2() {
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
            node->init = read_expr_stmt();
            expect(";");
        }
        if (!consume(";")) {
            node->cond = expr();
            expect(";");
        }
        if (!consume(")")) {
            node->inc = read_expr_stmt();
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

    if ((tok = peek("int")))
        return declaration();

    Node *node = read_expr_stmt();
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

static Node *new_add(Node *lhs, Node *rhs, Token *tok) {
    add_type(lhs);
    add_type(rhs);

    if (is_integer(lhs->ty) && is_integer(rhs->ty))
        return new_binary(ND_ADD, lhs, rhs, tok);
    if (lhs->ty->base && is_integer(rhs->ty))
        return new_binary(ND_PTR_ADD, lhs, rhs, tok);
    if (is_integer(lhs->ty) && rhs->ty->base)
        return new_binary(ND_PTR_ADD, rhs, lhs, tok);
    error_tok(tok, "invalid operands");
}

static Node *new_sub(Node *lhs, Node *rhs, Token *tok) {
    add_type(lhs);
    add_type(rhs);

    if (is_integer(lhs->ty) && is_integer(rhs->ty))
        return new_binary(ND_SUB, lhs, rhs, tok);
    if (lhs->ty->base && is_integer(rhs->ty))
        return new_binary(ND_PTR_SUB, lhs, rhs, tok);
    if (is_integer(rhs->ty) && lhs->ty->base)
        return new_binary(ND_PTR_SUB, rhs, lhs, tok);
    if (lhs->ty->base && rhs->ty->base)
        return new_binary(ND_PTR_DIFF, lhs, rhs, tok);
    error_tok(tok, "invalid operands");
}

static Node *add() {
    Node *node = mul();
    Token *tok;

    for(;;) {
        if ((tok = consume("+"))) {
            node = new_add(node, mul(), tok);
            continue;
        }
        if ((tok = consume("-"))) {
            node = new_sub(node, mul(), tok);
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

int size(Type *type) {
    if (type->kind == TY_INT)
        return 4;
    if (type->kind == TY_PTR)
        return 8;
    error("有効な型ではありません");
    return 0;
}

// unary = ("sizeof" | "+" | "-" | "*" | "&" )? unary
//       | postfix
static Node *unary() {
    Token *tok;
    if ((tok = consume_sizeof())) {
        Node *node = expr();
        add_type(node);
        return new_num(size(node->ty), tok);
    }
    if (consume("+"))
        return unary();
    if ((tok = consume("-")))
        return new_binary(ND_SUB, new_num(0, tok), unary(), tok);
    if ((tok = consume("&")))
        return new_unary(ND_ADDR, unary(), tok);
    if ((tok = consume("*")))
        return new_unary(ND_DEREF, unary(), tok);
    return postfix();
}

// postfix = primary ("[" expr "]")*
static Node *postfix() {
    Node *node = primary();
    Token *tok;

    while ((tok = consume("["))) {
        Node *exp = new_add(node, expr(), tok);
        expect("]");
        node = new_unary(ND_DEREF, exp, tok);
    }
    return node;
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
            error_tok(tok, "undefined variable");
        return new_var_node(var, tok);
    }

    tok = token;
    if (tok->kind != TK_NUM) {
        error_tok(tok, "式ではありません");
    }

    return new_num(expect_number(), tok);
}
