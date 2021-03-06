#include "9cc.h"

char *user_input;
char *filename;
Token *token;

void error(char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    fprintf(stderr, "\n");
    exit(1);
}

static void verror_at(char *loc, char *fmt, va_list ap) {

    char *line = loc;
    while (user_input < line && line[-1] != '\n')
        line--;

    char *end = loc;
    while (*end != '\n')
        end++;

    int line_num = 1;
    for (char *p = user_input; p < line; p++)
        if (*p == '\n')
            line_num++;

    int indent = fprintf(stderr, "%s:%d: ", filename, line_num);
    fprintf(stderr, "%.*s\n", (int)(end - line), line);

    int pos = loc - line + indent;
    fprintf(stderr, "%s\n", user_input);
    fprintf(stderr, "%*s^ ", pos, "");
    vfprintf(stderr, fmt, ap);
    fprintf(stderr, "\n");
    exit(1);
}

void error_at(char *loc, char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    verror_at(loc, fmt, ap);
}

void error_tok(Token *tok, char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    verror_at(tok->str, fmt, ap);
}

Token *consume(char *op) {
    if (token->kind != TK_RESERVED || 
        strlen(op) != token->len ||
        memcmp(token->str, op, token->len))
        return NULL;
    Token *t = token;
    token = token->next;
    return t;
}

Token *peek(char *s) {
    if (token->kind != TK_RESERVED || strlen(s) != token->len ||
            strncmp(token->str, s, token->len))
        return NULL;
    return token;
}

Token *consume_ident() {
    if (token->kind != TK_INDENT)
        return NULL;
    Token *t = token;
    token = token->next;
    return t;
}

long expect_number() {
    if (token->kind != TK_NUM)
        error_tok(token, "数値ではありません");
    long val = token->val;
    token = token->next;
    return val;
}

char *expect_ident() {
    if (token->kind != TK_INDENT)
        error_tok(token, "識別子ではありません");
    char *s = strndup(token->str, token->len);
    token = token->next;
    return s;
}


void expect(char *op) {
    if (!peek(op))
        error_tok(token, "'%s'ではありません", op);
    token = token->next;
}


static Token *new_token(TokenKind kind, Token *cur, char *str, int len) {
    Token *tok = calloc(1, sizeof(Token));
    tok->kind = kind;
    tok->str = str;
    tok->len = len;
    cur->next = tok;
    return tok;
}

bool startswith(char *p, char *q) {
    return memcmp(p, q, strlen(q)) == 0;
}

bool at_eof() {
    return token->kind == TK_EOF;
}

static char *starts_with_reserved(char *p) {
    static char *kw[] = {"return", "if", "else", "while", "for", "int", "sizeof", "char"};
    for (int i = 0; i < sizeof(kw) / sizeof(*kw); i++) {
        int len = strlen(kw[i]);
        if (startswith(p, kw[i]) && !isalnum(p[len])) {
            return kw[i];
        }
    }

    return NULL;
}

static char get_escape_char(char c) {
    switch (c) {
        case 'a': return '\a';
        case 'b': return '\b';
        case 't': return '\t';
        case 'n': return '\n';
        case 'v': return '\v';
        case 'f': return '\f';
        case 'r': return '\r';
        case 'e': return 27;
        case '0': return 0;
        default: return c;
    }
}

static Token *read_string_literal(Token *cur, char *start) {
    char *p = start + 1;
    char buf[1024];
    int len = 0;

    for (;;) {
        if (len == sizeof(buf))
            error_at(start, "string literal too large");
        if (*p == '\0')
            error_at(start, "unclosed string literal");
        if (*p == '"')
            break;

        if (*p == '\\') {
            p++;
            buf[len++] = get_escape_char(*p++);
        } else {
            buf[len++] = *p++;
        }
    }

    Token *tok = new_token(TK_STR, cur, start, p - start + 1);
    tok->contents = malloc(len + 1);
    memcpy(tok->contents, buf, len);
    tok->contents[len] = '\0';
    tok->cont_len = len + 1;
    return tok;
}

Token *tokenize(char *p) {
    user_input = p;
    Token head;
    head.next = NULL;
    Token *cur = &head;

    while (*p) {
        if (isspace(*p)) {
            p++;
            continue;
        }
        if (isdigit(*p)) {
            char *q = p;
            cur = new_token(TK_NUM, cur, p, 0);
            cur->val = strtol(p, &p, 10);
            cur->len = p - q;
            continue;
        }

        if (startswith(p, "//")) {
            p += 2;
            while (*p != '\n')
                p++;
            continue;
        }

        if (startswith(p, "/*")) {
            char *q = strstr(p + 2, "*/");
            if (!q)
                error_at(p, "コメントが閉じられてません");
            p = q + 2;
            continue;
        }

        if (startswith(p, "==") || startswith(p, "!=") || startswith(p, "<=") || startswith(p, ">=")) {
            cur = new_token(TK_RESERVED, cur, p, 2);
            p += 2;
            continue;
        }
        if (strchr("+-*/()<>;={},*&[]", *p)) {
            cur = new_token(TK_RESERVED, cur, p, 1);
            cur->val = *p;
            p++;
            continue;
        }
        char *kw = starts_with_reserved(p);
        if (kw) {
            int len = strlen(kw);
            cur = new_token(TK_RESERVED, cur, p, len);
            p += len;
            continue;
        }

        if (*p == '"') {
            cur = read_string_literal(cur, p);
            p += cur->len;
            continue;
        }

        if (isalnum(*p)) {
            char *q = p++;
            while(isalnum(*p))
                p++;
            cur = new_token(TK_INDENT, cur, q, p - q);
            continue;
        }

        error_at(p, "予期しない文字列です");
    }
    new_token(TK_EOF, cur, p, 0);
    return head.next;
}

