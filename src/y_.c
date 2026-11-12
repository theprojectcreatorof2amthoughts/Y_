#include <ctype.h>
#include <errno.h>
#include <math.h>
#include <limits.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "file_io.h"
#include "memory.h"
#include "embedded_libraries.h"
#include "y_.h"

#define Y_VERSION "0.3.0"

typedef enum {
    T_EOF, T_NAME, T_CUSTOM, T_NUMBER, T_STRING, T_LPAREN, T_RPAREN,
    T_COMMA, T_SEMI, T_PLUS, T_MINUS, T_STAR, T_SLASH, T_PERCENT,
    T_ASSIGN, T_EQ, T_NE, T_LT, T_LE, T_GT, T_GE, T_POWER, T_IDIV, T_CONCAT, T_OUTPUT, T_ERROR_OUTPUT,
    T_LIBINC, T_EMBEDDED_LIBRARY
} TokenKind;

typedef struct { TokenKind kind; char *text; double number; int line; } Token;
typedef struct { Token *items; size_t count, capacity, at; const char *path; } Parser;
typedef struct Expr Expr;
typedef struct Stmt Stmt;
typedef struct Function Function;
typedef struct Env Env;

typedef enum { V_NIL, V_NUMBER, V_STRING, V_BOOL, V_FUNCTION, V_MEMORY } ValueKind;
typedef struct {
    ValueKind kind;
    union { double number; char *string; bool boolean; Function *function; Memory *memory; } as;
} Value;

struct Function { char *name; char **params; size_t param_count; Stmt *body; Env *closure; };
struct Env { char **names; Value *values; size_t count, capacity; Env *parent; };

typedef enum { E_VALUE, E_VARIABLE, E_UNARY, E_BINARY, E_CALL } ExprKind;
struct Expr {
    ExprKind kind;
    int line;
    union {
        Value value;
        char *name;
        struct { char *op; Expr *right; } unary;
        struct { char *op; Expr *left, *right; } binary;
        struct { Expr *callee; Expr **args; size_t count; } call;
    } as;
};

typedef enum { S_LOCAL, S_ASSIGN, S_EXPR, S_OUTPUT, S_ERROR_OUTPUT, S_MEMORY, S_INCLUDE,
               S_EMBEDDED_INCLUDE,
               S_FUNCTION, S_RETURN, S_IF, S_WHILE, S_BREAK } StmtKind;
struct Stmt {
    StmtKind kind;
    int line;
    Stmt *next;
    union {
        struct { char *name; Expr *value; } variable;
        Expr *expression;
        struct { char *name; Expr *size; } memory;
        char *embedded_library;
        struct { char *name; char **params; size_t count; Stmt *body; } function;
        struct { Expr *value; } returned;
        struct { Expr *condition; Stmt *yes, *no; } conditional;
        struct { Expr *condition; Stmt *body; } loop;
    } as;
};

static bool failed;
static const char *active_path = "<input>";

static void report_error(int line, const char *message) {
    if (!failed) fprintf(stderr, "%s:%d: %s\n", active_path, line, message);
    failed = true;
}

static char *copy_range(const char *start, size_t length) {
    char *result = malloc(length + 1);
    if (!result) { fputs("out of memory\n", stderr); exit(2); }
    memcpy(result, start, length);
    result[length] = '\0';
    return result;
}

static void add_token(Parser *parser, Token token) {
    if (parser->count == parser->capacity) {
        parser->capacity = parser->capacity ? parser->capacity * 2 : 64;
        parser->items = realloc(parser->items, parser->capacity * sizeof(*parser->items));
        if (!parser->items) { fputs("out of memory\n", stderr); exit(2); }
    }
    parser->items[parser->count++] = token;
}

static bool is_name_start(char c) { return isalpha((unsigned char)c) || c == '_'; }
static bool is_name_char(char c) { return isalnum((unsigned char)c) || c == '_'; }

static void tokenize(Parser *parser, const char *source) {
    const char *cursor = source;
    int line = 1;
    while (*cursor && !failed) {
        if (*cursor == '\n') { ++line; ++cursor; continue; }
        if (isspace((unsigned char)*cursor)) { ++cursor; continue; }
        if (cursor[0] == '#' && cursor[1] == '#') {
            while (*cursor && *cursor != '\n') ++cursor;
            continue;
        }
        Token token = { .line = line };
        if (cursor[0] == '!' && cursor[1] == '$') {
            cursor += 2;
            if (*cursor == '{') {
                ++cursor;
                size_t capacity = strlen(cursor) + 1;
                char *name = malloc(capacity);
                if (!name) { fputs("out of memory\n", stderr); exit(2); }
                size_t length = 0;
                while (*cursor && *cursor != '}' && *cursor != '\n') {
                    if (*cursor == '\\' && (cursor[1] == '}' || cursor[1] == '\\')) ++cursor;
                    name[length++] = *cursor++;
                }
                if (*cursor != '}') {
                    free(name);
                    report_error(line, "expected '}' to close custom variable name");
                    break;
                }
                ++cursor;
                if (length == 0) {
                    free(name);
                    report_error(line, "custom variable name cannot be empty");
                    break;
                }
                name[length] = '\0';
                token.kind = T_CUSTOM;
                token.text = name;
                add_token(parser, token);
                continue;
            }
            const char *start = cursor;
            if (!is_name_start(*cursor)) { report_error(line, "expected a name after !$ or a braced name after !${"); break; }
            while (is_name_char(*cursor)) ++cursor;
            token.kind = T_CUSTOM;
            token.text = copy_range(start, (size_t)(cursor - start));
            add_token(parser, token);
            continue;
        }
        if (cursor[0] == '!' && cursor[1] == '#') {
            cursor += 2;
            const char *start = cursor;
            while (is_name_char(*cursor)) ++cursor;
            size_t directive_length = (size_t)(cursor - start);
            if (directive_length == 7 && strncmp(start, "inptype", 7) == 0) {
                while (*cursor == ' ' || *cursor == '\\t') ++cursor;
                if (*cursor != '(') {
                    report_error(line, "expected '(' after !#inptype");
                    break;
                }
                token.kind = T_NAME;
                token.text = copy_range("input", 5);
                add_token(parser, token);
                continue;
            }
            if (directive_length == 6 && strncmp(start, "libinc", 6) == 0) {
                token.kind = T_LIBINC;
                token.text = copy_range(start, directive_length);
                add_token(parser, token);
                continue;
            }
            if (directive_length != 6 || strncmp(start, "inclib", 6) != 0) {
                report_error(line, "unknown directive; expected !#libinc or !#inclib");
                break;
            }
            while (*cursor == ' ' || *cursor == '\t') ++cursor;
            if (*cursor != '%') { report_error(line, "expected '%' before embedded .yl library path"); break; }
            ++cursor;
            const char *path_start = cursor;
            while (*cursor && *cursor != '%' && *cursor != '\n') ++cursor;
            if (*cursor != '%' || cursor == path_start) {
                report_error(line, "expected a non-empty .yl path between '%' markers");
                break;
            }
            token.kind = T_EMBEDDED_LIBRARY;
            token.text = copy_range(path_start, (size_t)(cursor - path_start));
            ++cursor;
            add_token(parser, token);
            continue;
        }
        if (is_name_start(*cursor)) {
            const char *start = cursor++;
            while (is_name_char(*cursor)) ++cursor;
            token.kind = T_NAME; token.text = copy_range(start, (size_t)(cursor - start));
            add_token(parser, token); continue;
        }
        if (isdigit((unsigned char)*cursor) || (*cursor == '.' && isdigit((unsigned char)cursor[1]))) {
            char *end = NULL;
            errno = 0;
            token.number = strtod(cursor, &end);
            if (errno == ERANGE || end == cursor) { report_error(line, "invalid number"); break; }
            token.kind = T_NUMBER; token.text = copy_range(cursor, (size_t)(end - cursor));
            cursor = end; add_token(parser, token); continue;
        }
        if (*cursor == '"' || *cursor == '\'') {
            char quote = *cursor++;
            size_t capacity = 32, length = 0;
            char *value = malloc(capacity);
            if (!value) { fputs("out of memory\n", stderr); exit(2); }
            while (*cursor && *cursor != quote) {
                char ch = *cursor++;
                if (ch == '\\' && *cursor) {
                    ch = *cursor++;
                    if (ch == 'n') ch = '\n'; else if (ch == 't') ch = '\t';
                } else if (ch == '\n') ++line;
                if (length + 1 >= capacity) { capacity *= 2; value = realloc(value, capacity); }
                value[length++] = ch;
            }
            if (*cursor != quote) { free(value); report_error(line, "unterminated string"); break; }
            ++cursor; value[length] = '\0'; token.kind = T_STRING; token.text = value;
            add_token(parser, token); continue;
        }
        ++cursor;
        switch (cursor[-1]) {
            case '(': token.kind = T_LPAREN; break; case ')': token.kind = T_RPAREN; break;
            case ',': token.kind = T_COMMA; break; case ';': token.kind = T_SEMI; break;
            case '+': token.kind = T_PLUS; break; case '-': token.kind = T_MINUS; break;
            case '*': if (*cursor == '*') { ++cursor; token.kind = T_POWER; } else token.kind = T_STAR; break;
            case '/': if (*cursor == '/') { ++cursor; token.kind = T_IDIV; } else token.kind = T_SLASH; break;
            case '.': if (*cursor == '.') { ++cursor; token.kind = T_CONCAT; } else { report_error(line, "a single '.' is not an operator"); continue; } break;
            case '%': token.kind = T_PERCENT; break;
            case '=': if (*cursor == '=') { ++cursor; token.kind = T_EQ; } else token.kind = T_ASSIGN; break;
                case '~': if (*cursor == '=') { ++cursor; token.kind = T_NE; } else { report_error(line, "expected ~="); continue; } break;
            case '<':
                if (*cursor == '^') { ++cursor; token.kind = T_ERROR_OUTPUT; }
                else if (*cursor == '=') { ++cursor; token.kind = T_LE; }
                else token.kind = T_LT;
                break;
            case '>':
                if (*cursor == '^') { ++cursor; token.kind = T_OUTPUT; }
                else if (*cursor == '=') { ++cursor; token.kind = T_GE; }
                else token.kind = T_GT;
                break;
            default: report_error(line, "unexpected character"); continue;
        }
        add_token(parser, token);
    }
    add_token(parser, (Token){ .kind = T_EOF, .line = line });
}

static Token *peek(Parser *p) { return &p->items[p->at]; }
static Token *previous(Parser *p) { return &p->items[p->at - 1]; }
static bool keyword(Parser *p, const char *name) { return peek(p)->kind == T_NAME && strcmp(peek(p)->text, name) == 0; }
static bool match(Parser *p, TokenKind kind) { if (peek(p)->kind != kind) return false; ++p->at; return true; }
static bool match_keyword(Parser *p, const char *name) { if (!keyword(p, name)) return false; ++p->at; return true; }

static bool expect(Parser *p, TokenKind kind, const char *message) {
    if (match(p, kind)) return true;
    report_error(peek(p)->line, message); return false;
}

static bool expect_keyword(Parser *p, const char *name) {
    if (match_keyword(p, name)) return true;
    report_error(peek(p)->line, "unexpected keyword or missing block delimiter"); return false;
}

static Expr *new_expr(ExprKind kind, int line) {
    Expr *expr = calloc(1, sizeof(*expr));
    if (!expr) { fputs("out of memory\n", stderr); exit(2); }
    expr->kind = kind; expr->line = line; return expr;
}

static Stmt *new_stmt(StmtKind kind, int line) {
    Stmt *stmt = calloc(1, sizeof(*stmt));
    if (!stmt) { fputs("out of memory\n", stderr); exit(2); }
    stmt->kind = kind; stmt->line = line; return stmt;
}

static Expr *parse_expression(Parser *p, int minimum);

static Expr *parse_primary(Parser *p) {
    Token token = *peek(p);
    if (match(p, T_NUMBER)) { Expr *e = new_expr(E_VALUE, token.line); e->as.value.kind = V_NUMBER; e->as.value.as.number = token.number; return e; }
    if (match(p, T_STRING)) { Expr *e = new_expr(E_VALUE, token.line); e->as.value.kind = V_STRING; e->as.value.as.string = token.text; return e; }
    if (match(p, T_CUSTOM) || match(p, T_NAME)) {
        bool custom = previous(p)->kind == T_CUSTOM;
        char *name = previous(p)->text;
        if (!custom && strcmp(name, "yes") == 0) { Expr *e = new_expr(E_VALUE, token.line); e->as.value.kind = V_BOOL; e->as.value.as.boolean = true; return e; }
        if (!custom && strcmp(name, "no") == 0) { Expr *e = new_expr(E_VALUE, token.line); e->as.value.kind = V_BOOL; e->as.value.as.boolean = false; return e; }
        if (!custom && strcmp(name, "void") == 0) { Expr *e = new_expr(E_VALUE, token.line); e->as.value.kind = V_NIL; return e; }
        Expr *e = new_expr(E_VARIABLE, token.line); e->as.name = name; return e;
    }
    if (match(p, T_LPAREN)) {
        Expr *e = parse_expression(p, 1);
        expect(p, T_RPAREN, "expected ')' after expression");
        return e;
    }
    report_error(token.line, "expected an expression");
    return new_expr(E_VALUE, token.line);
}

static Expr *parse_prefix(Parser *p) {
    if (match(p, T_MINUS) || match_keyword(p, "invert")) {
        Token *op = previous(p);
        Expr *e = new_expr(E_UNARY, op->line);
        e->as.unary.op = op->kind == T_MINUS ? "-" : "invert";
        e->as.unary.right = parse_expression(p, 7);
        return e;
    }
    Expr *expr = parse_primary(p);
    while (match(p, T_LPAREN)) {
        Expr *call = new_expr(E_CALL, expr->line);
        call->as.call.callee = expr;
        if (!match(p, T_RPAREN)) {
            do {
                Expr *arg = parse_expression(p, 1);
                call->as.call.args = realloc(call->as.call.args, (call->as.call.count + 1) * sizeof(*call->as.call.args));
                call->as.call.args[call->as.call.count++] = arg;
            } while (match(p, T_COMMA));
            expect(p, T_RPAREN, "expected ')' after arguments");
        }
        expr = call;
    }
    return expr;
}

static int precedence(Parser *p, const char **op) {
    TokenKind kind = peek(p)->kind;
    if (kind == T_NAME && strcmp(peek(p)->text, "define") == 0 &&
        p->at + 1 < p->count && p->items[p->at + 1].kind == T_ASSIGN) {
        *op = "define=";
        return 3;
    }
    if (kind == T_NAME && (strcmp(peek(p)->text, "either") == 0 || strcmp(peek(p)->text, "both") == 0)) {
        *op = peek(p)->text; return strcmp(*op, "either") == 0 ? 1 : 2;
    }
    switch (kind) {
        case T_EQ: *op = "=="; return 3; case T_NE: *op = "~="; return 3;
        case T_LT: *op = "<"; return 3; case T_LE: *op = "<="; return 3;
        case T_GT: *op = ">"; return 3; case T_GE: *op = ">="; return 3;
        case T_PLUS: *op = "+"; return 4; case T_MINUS: *op = "-"; return 4;
        case T_CONCAT: *op = ".."; return 4;
        case T_STAR: *op = "*"; return 5; case T_SLASH: *op = "/"; return 5;
        case T_IDIV: *op = "//"; return 5; case T_PERCENT: *op = "%"; return 5;
        case T_POWER: *op = "**"; return 6; default: return 0;
    }
}

static Expr *parse_expression(Parser *p, int minimum) {
    Expr *left = parse_prefix(p);
    while (!failed) {
        const char *op = NULL;
        int level = precedence(p, &op);
        if (level < minimum || level == 0) break;
        if (strcmp(op, "define=") == 0) p->at += 2;
        else ++p->at;
        Expr *right = parse_expression(p, strcmp(op, "**") == 0 ? level : level + 1);
        Expr *binary = new_expr(E_BINARY, left->line);
        binary->as.binary.op = (char *)op;
        binary->as.binary.left = left;
        binary->as.binary.right = right;
        left = binary;
    }
    return left;
}

static Stmt *parse_block(Parser *p, bool stop_at_else) {
    Stmt *head = NULL, **tail = &head;
    while (peek(p)->kind != T_EOF && !keyword(p, "done") && !(stop_at_else && keyword(p, "else")) && !failed) {
        if (match(p, T_SEMI)) continue;
        size_t before = p->at;
        Stmt *stmt = NULL;
        Token start = *peek(p);
        if (match_keyword(p, "stash")) {
            if (!match(p, T_NAME) && !match(p, T_CUSTOM)) {
                report_error(peek(p)->line, "expected a variable name after stash");
                break;
            }
            Token name = *previous(p);
            stmt = new_stmt(S_LOCAL, start.line); stmt->as.variable.name = name.text;
            if (match(p, T_ASSIGN)) stmt->as.variable.value = parse_expression(p, 1);
        } else if (match_keyword(p, "define")) {
            Token name = *peek(p);
            if (!expect(p, T_NAME, "expected a procedure name")) break;
            stmt = new_stmt(S_FUNCTION, start.line); stmt->as.function.name = name.text;
            expect(p, T_LPAREN, "expected '(' after procedure name");
            if (!match(p, T_RPAREN)) {
                do {
                    Token param = *peek(p);
                    if (!expect(p, T_NAME, "expected a parameter name")) break;
                    stmt->as.function.params = realloc(stmt->as.function.params, (stmt->as.function.count + 1) * sizeof(char *));
                    stmt->as.function.params[stmt->as.function.count++] = param.text;
                } while (match(p, T_COMMA));
                expect(p, T_RPAREN, "expected ')' after parameters");
            }
            stmt->as.function.body = parse_block(p, false);
            expect_keyword(p, "done");
        } else if (match_keyword(p, "when")) {
            stmt = new_stmt(S_IF, start.line);
            stmt->as.conditional.condition = parse_expression(p, 1);
            expect_keyword(p, "doit");
            stmt->as.conditional.yes = parse_block(p, true);
            if (match_keyword(p, "else")) stmt->as.conditional.no = parse_block(p, false);
            expect_keyword(p, "done");
        } else if (match_keyword(p, "repeatwhile")) {
            stmt = new_stmt(S_WHILE, start.line);
            stmt->as.loop.condition = parse_expression(p, 1);
            expect_keyword(p, "execute");
            stmt->as.loop.body = parse_block(p, false);
            expect_keyword(p, "done");
        } else if (match_keyword(p, "give")) {
            stmt = new_stmt(S_RETURN, start.line);
            if (peek(p)->kind != T_SEMI && !keyword(p, "done") && peek(p)->kind != T_EOF)
                stmt->as.returned.value = parse_expression(p, 1);
        } else if (match_keyword(p, "escape")) {
            stmt = new_stmt(S_BREAK, start.line);
        } else if (match_keyword(p, "writescrn")) {
            stmt = new_stmt(S_OUTPUT, start.line);
            expect(p, T_OUTPUT, "expected >^ after writescrn");
            stmt->as.expression = parse_expression(p, 1);
        } else if (match_keyword(p, "errstate")) {
            stmt = new_stmt(S_ERROR_OUTPUT, start.line);
            expect(p, T_ERROR_OUTPUT, "expected <^ after errstate");
            stmt->as.expression = parse_expression(p, 1);
        } else if (match_keyword(p, "memdesig")) {
            Token name = *peek(p);
            if (!match(p, T_CUSTOM) && !match(p, T_NAME)) report_error(peek(p)->line, "expected a variable name after memdesig");
            else name = *previous(p);
            stmt = new_stmt(S_MEMORY, start.line); stmt->as.memory.name = name.text;
            if (!match(p, T_COMMA) && !match(p, T_ASSIGN)) report_error(peek(p)->line, "expected ',' before memory size");
            stmt->as.memory.size = parse_expression(p, 1);
        } else if (match(p, T_LIBINC)) {
            stmt = new_stmt(S_INCLUDE, start.line);
            stmt->as.expression = parse_expression(p, 1);
        } else if (match(p, T_EMBEDDED_LIBRARY)) {
            stmt = new_stmt(S_EMBEDDED_INCLUDE, start.line);
            stmt->as.embedded_library = previous(p)->text;
        } else if (peek(p)->kind == T_CUSTOM || peek(p)->kind == T_NAME) {
            Token name = *peek(p); ++p->at;
            if (match(p, T_ASSIGN)) {
                stmt = new_stmt(S_ASSIGN, start.line); stmt->as.variable.name = name.text;
                stmt->as.variable.value = parse_expression(p, 1);
            } else {
                --p->at;
                stmt = new_stmt(S_EXPR, start.line); stmt->as.expression = parse_expression(p, 1);
            }
        } else {
            report_error(peek(p)->line, "unexpected token at start of statement"); break;
        }
        if (stmt) { *tail = stmt; tail = &stmt->next; }
        match(p, T_SEMI);
        if (p->at == before) { report_error(peek(p)->line, "parser made no progress"); break; }
    }
    return head;
}

static Env *new_env(Env *parent) { Env *env = calloc(1, sizeof(*env)); if (!env) exit(2); env->parent = parent; return env; }

static Value nil_value(void) { return (Value){ .kind = V_NIL }; }

static Value *find_value(Env *env, const char *name) {
    for (Env *scope = env; scope; scope = scope->parent)
        for (size_t i = 0; i < scope->count; ++i)
            if (strcmp(scope->names[i], name) == 0) return &scope->values[i];
    return NULL;
}

static void define_value(Env *env, const char *name, Value value) {
    for (size_t i = 0; i < env->count; ++i) if (strcmp(env->names[i], name) == 0) { env->values[i] = value; return; }
    if (env->count == env->capacity) {
        env->capacity = env->capacity ? env->capacity * 2 : 8;
        env->names = realloc(env->names, env->capacity * sizeof(char *));
        env->values = realloc(env->values, env->capacity * sizeof(Value));
    }
    env->names[env->count] = (char *)name; env->values[env->count++] = value;
}

static bool truthy(Value value) { return value.kind != V_NIL && !(value.kind == V_BOOL && !value.as.boolean); }

static void write_value(FILE *out, Value value) {
    switch (value.kind) {
        case V_NIL: fputs("void", out); break;
        case V_NUMBER: if (value.as.number == floor(value.as.number)) fprintf(out, "%.0f", value.as.number); else fprintf(out, "%.15g", value.as.number); break;
        case V_STRING: fputs(value.as.string, out); break;
        case V_BOOL: fputs(value.as.boolean ? "yes" : "no", out); break;
        case V_FUNCTION: fputs("<function>", out); break;
        case V_MEMORY: fprintf(out, "<memory:%zu>", value.as.memory->size); break;
    }
}

typedef struct { bool returned, broke; Value value; } Result;
static Value evaluate(Expr *expr, Env *env);
static Result execute(Stmt *stmt, Env *env);
static Result execute_list(Stmt *stmt, Env *env);
static bool run_source(const char *source, const char *path, Env *env);

static bool number_value(Value value, double *out, int line) {
    if (value.kind != V_NUMBER) { report_error(line, "expected a number"); return false; }
    *out = value.as.number; return true;
}

static Value read_input_line(int line) {
    size_t capacity = 64, length = 0;
    char *buffer = malloc(capacity);
    if (!buffer) { fputs("out of memory\n", stderr); exit(2); }
    int character;
    while ((character = getchar()) != EOF && character != '\n') {
        if (length + 1 >= capacity) {
            capacity *= 2;
            char *grown = realloc(buffer, capacity);
            if (!grown) { free(buffer); fputs("out of memory\n", stderr); exit(2); }
            buffer = grown;
        }
        buffer[length++] = (char)character;
    }
    if (character == EOF && ferror(stdin)) {
        free(buffer);
        report_error(line, "failed to read input");
        return nil_value();
    }
    if (character == EOF && length == 0) { free(buffer); return nil_value(); }
    if (length > 0 && buffer[length - 1] == '\r') --length;
    buffer[length] = '\0';
    return (Value){ .kind = V_STRING, .as.string = buffer };
}

static Value call_function(Expr *expr, Env *env) {
    bool builtin = false;
    if (expr->as.call.callee->kind == E_VARIABLE) {
        const char *name = expr->as.call.callee->as.name;
        builtin = strcmp(name, "memread") == 0 || strcmp(name, "memwrite") == 0 || strcmp(name, "tostring") == 0 || strcmp(name, "input") == 0 || strcmp(name, "type") == 0 || strcmp(name, "tonumber") == 0 || strcmp(name, "length") == 0 || strcmp(name, "abs") == 0 || strcmp(name, "sqrt") == 0 || strcmp(name, "floor") == 0 || strcmp(name, "ceil") == 0 || strcmp(name, "round") == 0 || strcmp(name, "pow") == 0 || strcmp(name, "min") == 0 || strcmp(name, "max") == 0 || strcmp(name, "sin") == 0 || strcmp(name, "cos") == 0 || strcmp(name, "tan") == 0 || strcmp(name, "log") == 0 || strcmp(name, "exp") == 0 || strcmp(name, "sign") == 0 || strcmp(name, "isnumber") == 0 || strcmp(name, "isstring") == 0 || strcmp(name, "isboolean") == 0 || strcmp(name, "isvoid") == 0 || strcmp(name, "upper") == 0 || strcmp(name, "lower") == 0 || strcmp(name, "contains") == 0 || strcmp(name, "startswith") == 0 || strcmp(name, "endswith") == 0 || strcmp(name, "substring") == 0 || strcmp(name, "replace") == 0 || strcmp(name, "repeat") == 0 || strcmp(name, "reverse") == 0 || strcmp(name, "charat") == 0 || strcmp(name, "isfinite") == 0 || strcmp(name, "isnan") == 0 || strcmp(name, "hypot") == 0 || strcmp(name, "atan2") == 0 || strcmp(name, "radians") == 0 || strcmp(name, "degrees") == 0 || strcmp(name, "sum") == 0 || strcmp(name, "average") == 0 || strcmp(name, "clamp") == 0 || strcmp(name, "lerp") == 0;
    }
    Value callee = builtin ? nil_value() : evaluate(expr->as.call.callee, env);
    Value *args = expr->as.call.count ? calloc(expr->as.call.count, sizeof(Value)) : NULL;
    for (size_t i = 0; i < expr->as.call.count; ++i) args[i] = evaluate(expr->as.call.args[i], env);
    if (callee.kind == V_FUNCTION) {
        Function *function = callee.as.function;
        Env *call_env = new_env(function->closure);
        for (size_t i = 0; i < function->param_count; ++i)
            define_value(call_env, function->params[i], i < expr->as.call.count ? args[i] : nil_value());
        Result result = execute_list(function->body, call_env);
        free(args);
        return result.returned ? result.value : nil_value();
    }
    if (expr->as.call.callee->kind == E_VARIABLE) {
        const char *name = expr->as.call.callee->as.name;
        if (strcmp(name, "input") == 0) {
            if (expr->as.call.count > 1) report_error(expr->line, "input expects zero arguments or one prompt string");
            else if (expr->as.call.count == 1 && args[0].kind != V_STRING) report_error(expr->line, "input prompt must be a string");
            else {
                if (expr->as.call.count == 1) {
                    fputs(args[0].as.string, stdout);
                    fflush(stdout);
                }
                Value result = read_input_line(expr->line);
                free(args);
                return result;
            }
        } else if ((strcmp(name, "isnumber") == 0 || strcmp(name, "isstring") == 0 ||
                    strcmp(name, "isboolean") == 0 || strcmp(name, "isvoid") == 0) && expr->as.call.count == 1) {
            bool matches = strcmp(name, "isnumber") == 0 ? args[0].kind == V_NUMBER :
                           strcmp(name, "isstring") == 0 ? args[0].kind == V_STRING :
                           strcmp(name, "isboolean") == 0 ? args[0].kind == V_BOOL : args[0].kind == V_NIL;
            Value result = { .kind = V_BOOL, .as.boolean = matches };
            free(args); return result;
        } else if ((strcmp(name, "upper") == 0 || strcmp(name, "lower") == 0) && expr->as.call.count == 1) {
            if (args[0].kind != V_STRING) report_error(expr->line, "upper/lower expects a string");
            else {
                size_t n = strlen(args[0].as.string);
                char *converted = copy_range(args[0].as.string, n);
                for (size_t i = 0; i < n; ++i)
                    converted[i] = (char)(strcmp(name, "upper") == 0 ?
                        toupper((unsigned char)converted[i]) : tolower((unsigned char)converted[i]));
                Value result = { .kind = V_STRING, .as.string = converted };
                free(args); return result;
            }
        } else if ((strcmp(name, "contains") == 0 || strcmp(name, "startswith") == 0 || strcmp(name, "endswith") == 0) && expr->as.call.count == 2) {
            if (args[0].kind != V_STRING || args[1].kind != V_STRING) report_error(expr->line, "string predicates expect two strings");
            else {
                const char *source = args[0].as.string, *part = args[1].as.string;
                size_t source_length = strlen(source), part_length = strlen(part);
                bool matches = strcmp(name, "contains") == 0 ? strstr(source, part) != NULL :
                               strcmp(name, "startswith") == 0 ? part_length <= source_length && strncmp(source, part, part_length) == 0 :
                               part_length <= source_length && strcmp(source + source_length - part_length, part) == 0;
                Value result = { .kind = V_BOOL, .as.boolean = matches };
                free(args); return result;
            }
        } else if (strcmp(name, "substring") == 0 && (expr->as.call.count == 2 || expr->as.call.count == 3)) {
            double start_number, count_number = -1;
            if (args[0].kind != V_STRING || !number_value(args[1], &start_number, expr->line) ||
                (expr->as.call.count == 3 && !number_value(args[2], &count_number, expr->line))) {
                if (args[0].kind != V_STRING) report_error(expr->line, "substring expects a string and numeric indexes");
            } else {
                size_t source_length = strlen(args[0].as.string);
                if (floor(start_number) != start_number || start_number < -(double)source_length || start_number > (double)source_length ||
                    (expr->as.call.count == 3 && (floor(count_number) != count_number || count_number < 0))) {
                    report_error(expr->line, "substring indexes must be whole numbers within the string");
                } else {
                    long start = (long)start_number;
                    if (start < 0) start += (long)source_length;
                    size_t available = source_length - (size_t)start;
                    size_t count = expr->as.call.count == 3 && count_number < (double)available ? (size_t)count_number : available;
                    Value result = { .kind = V_STRING, .as.string = copy_range(args[0].as.string + start, count) };
                    free(args); return result;
                }
            }
        } else if (strcmp(name, "charat") == 0 && expr->as.call.count == 2) {
            double index_number;
            if (args[0].kind != V_STRING || !number_value(args[1], &index_number, expr->line)) {
                if (args[0].kind != V_STRING) report_error(expr->line, "charat expects a string and an index");
            } else {
                size_t source_length = strlen(args[0].as.string);
                if (floor(index_number) != index_number || index_number < -(double)source_length || index_number >= (double)source_length) report_error(expr->line, "charat index is outside the string");
                else {
                    long index = (long)index_number;
                    if (index < 0) index += (long)source_length;
                    Value result = { .kind = V_STRING, .as.string = copy_range(args[0].as.string + index, 1) };
                    free(args); return result;
                }
            }
        } else if (strcmp(name, "replace") == 0 && expr->as.call.count == 3) {
            if (args[0].kind != V_STRING || args[1].kind != V_STRING || args[2].kind != V_STRING) report_error(expr->line, "replace expects three strings");
            else {
                const char *source = args[0].as.string, *needle = args[1].as.string, *replacement = args[2].as.string;
                size_t source_length = strlen(source), needle_length = strlen(needle), replacement_length = strlen(replacement), occurrences = 0;
                if (needle_length == 0) report_error(expr->line, "replace search text cannot be empty");
                else {
                    for (const char *scan = source; (scan = strstr(scan, needle)) != NULL; scan += needle_length) ++occurrences;
                    size_t output_length = source_length;
                    if (replacement_length >= needle_length) {
                        size_t growth = replacement_length - needle_length;
                        if (growth && occurrences > (16777216u - source_length) / growth) output_length = 16777217u;
                        else output_length += occurrences * growth;
                    } else output_length -= occurrences * (needle_length - replacement_length);
                    if (output_length > 16777216u) report_error(expr->line, "replace result exceeds 16 MiB");
                    else {
                        char *output = malloc(output_length + 1);
                        if (!output) { fputs("out of memory\n", stderr); exit(2); }
                        const char *scan = source;
                        char *write = output;
                        const char *match;
                        while ((match = strstr(scan, needle)) != NULL) {
                            size_t prefix = (size_t)(match - scan);
                            memcpy(write, scan, prefix); write += prefix;
                            memcpy(write, replacement, replacement_length); write += replacement_length;
                            scan = match + needle_length;
                        }
                        size_t tail = strlen(scan); memcpy(write, scan, tail); write += tail; *write = '\0';
                        Value result = { .kind = V_STRING, .as.string = output };
                        free(args); return result;
                    }
                }
            }
        } else if (strcmp(name, "repeat") == 0 && expr->as.call.count == 2) {
            double count_number;
            if (args[0].kind != V_STRING || !number_value(args[1], &count_number, expr->line)) {
                if (args[0].kind != V_STRING) report_error(expr->line, "repeat expects a string and a count");
            } else {
                size_t source_length = strlen(args[0].as.string);
                if (floor(count_number) != count_number || count_number < 0 || count_number > 1000000 ||
                    (source_length && count_number > 16777216.0 / (double)source_length)) report_error(expr->line, "repeat count or result size is out of range");
                else {
                    size_t count = (size_t)count_number, output_length = source_length * count;
                    char *output = malloc(output_length + 1);
                    if (!output) { fputs("out of memory\n", stderr); exit(2); }
                    for (size_t i = 0; i < count; ++i) memcpy(output + i * source_length, args[0].as.string, source_length);
                    output[output_length] = '\0';
                    Value result = { .kind = V_STRING, .as.string = output };
                    free(args); return result;
                }
            }
        } else if (strcmp(name, "reverse") == 0 && expr->as.call.count == 1) {
            if (args[0].kind != V_STRING) report_error(expr->line, "reverse expects a string");
            else {
                size_t n = strlen(args[0].as.string);
                char *output = copy_range(args[0].as.string, n);
                for (size_t i = 0; i < n / 2; ++i) { char swap = output[i]; output[i] = output[n - i - 1]; output[n - i - 1] = swap; }
                Value result = { .kind = V_STRING, .as.string = output };
                free(args); return result;
            }
        } else if ((strcmp(name, "isfinite") == 0 || strcmp(name, "isnan") == 0) && expr->as.call.count == 1) {
            if (args[0].kind != V_NUMBER) report_error(expr->line, "isfinite/isnan expects a number");
            else {
                Value result = { .kind = V_BOOL, .as.boolean = strcmp(name, "isfinite") == 0 ? isfinite(args[0].as.number) : isnan(args[0].as.number) };
                free(args); return result;
            }
        } else if ((strcmp(name, "sum") == 0 || strcmp(name, "average") == 0) && expr->as.call.count > 0) {
            double total = 0;
            for (size_t i = 0; i < expr->as.call.count; ++i) {
                double number;
                if (!number_value(args[i], &number, expr->line)) { free(args); return nil_value(); }
                total += number;
            }
            if (!isfinite(total)) report_error(expr->line, "sum/average produced a non-finite result");
            else {
                Value result = { .kind = V_NUMBER, .as.number = strcmp(name, "average") == 0 ? total / (double)expr->as.call.count : total };
                free(args); return result;
            }
        } else if (strcmp(name, "clamp") == 0 && expr->as.call.count == 3) {
            double value, low, high;
            if (!number_value(args[0], &value, expr->line) || !number_value(args[1], &low, expr->line) ||
                !number_value(args[2], &high, expr->line)) { free(args); return nil_value(); }
            if (low > high) report_error(expr->line, "clamp lower bound exceeds upper bound");
            else {
                Value result = { .kind = V_NUMBER, .as.number = fmin(fmax(value, low), high) };
                free(args); return result;
            }
        } else if (strcmp(name, "lerp") == 0 && expr->as.call.count == 3) {
            double from, to, amount;
            if (!number_value(args[0], &from, expr->line) || !number_value(args[1], &to, expr->line) ||
                !number_value(args[2], &amount, expr->line)) { free(args); return nil_value(); }
            Value result = { .kind = V_NUMBER, .as.number = from + (to - from) * amount };
            free(args); return result;
        } else if (strcmp(name, "type") == 0 && expr->as.call.count == 1) {
            const char *type_name = "void";
            switch (args[0].kind) {
                case V_NIL: type_name = "void"; break;
                case V_NUMBER: type_name = "number"; break;
                case V_STRING: type_name = "string"; break;
                case V_BOOL: type_name = "boolean"; break;
                case V_FUNCTION: type_name = "function"; break;
                case V_MEMORY: type_name = "memory"; break;
            }
            Value result = { .kind = V_STRING, .as.string = copy_range(type_name, strlen(type_name)) };
            free(args); return result;
        } else if (strcmp(name, "length") == 0 && expr->as.call.count == 1) {
            if (args[0].kind != V_STRING) report_error(expr->line, "length expects a string");
            else {
                Value result = { .kind = V_NUMBER, .as.number = (double)strlen(args[0].as.string) };
                free(args); return result;
            }
        } else if (strcmp(name, "tonumber") == 0 && expr->as.call.count == 1) {
            if (args[0].kind == V_NUMBER) { Value result = args[0]; free(args); return result; }
            if (args[0].kind != V_STRING) report_error(expr->line, "tonumber expects a string or number");
            else {
                char *end = NULL;
                errno = 0;
                double parsed = strtod(args[0].as.string, &end);
                while (end && isspace((unsigned char)*end)) ++end;
                if (errno == ERANGE || end == args[0].as.string || !end || *end) report_error(expr->line, "tonumber could not parse the string");
                else { Value result = { .kind = V_NUMBER, .as.number = parsed }; free(args); return result; }
            }
        } else if ((strcmp(name, "abs") == 0 || strcmp(name, "sqrt") == 0 ||
                    strcmp(name, "floor") == 0 || strcmp(name, "ceil") == 0 ||
                    strcmp(name, "round") == 0 || strcmp(name, "sin") == 0 ||
                    strcmp(name, "cos") == 0 || strcmp(name, "tan") == 0 ||
                    strcmp(name, "log") == 0 || strcmp(name, "exp") == 0 || strcmp(name, "sign") == 0) && expr->as.call.count == 1) {
            double x;
            if (!number_value(args[0], &x, expr->line)) { free(args); return nil_value(); }
            double result_number = x;
            if (strcmp(name, "abs") == 0) result_number = fabs(x);
            else if (strcmp(name, "sqrt") == 0) result_number = sqrt(x);
            else if (strcmp(name, "floor") == 0) result_number = floor(x);
            else if (strcmp(name, "ceil") == 0) result_number = ceil(x);
            else if (strcmp(name, "round") == 0) result_number = round(x);
            else if (strcmp(name, "sin") == 0) result_number = sin(x);
            else if (strcmp(name, "cos") == 0) result_number = cos(x);
            else if (strcmp(name, "tan") == 0) result_number = tan(x);
            else if (strcmp(name, "log") == 0) result_number = log(x);
            else if (strcmp(name, "exp") == 0) result_number = exp(x);
            else if (strcmp(name, "sign") == 0) result_number = (x > 0) - (x < 0);
            if (!isfinite(result_number)) report_error(expr->line, "math function produced a non-finite result");
            else { Value result = { .kind = V_NUMBER, .as.number = result_number }; free(args); return result; }
        } else if ((strcmp(name, "pow") == 0 || strcmp(name, "min") == 0 || strcmp(name, "max") == 0) && expr->as.call.count == 2) {
            double x, y;
            if (!number_value(args[0], &x, expr->line) || !number_value(args[1], &y, expr->line)) { free(args); return nil_value(); }
            double result_number = strcmp(name, "pow") == 0 ? pow(x, y) :
                                   (strcmp(name, "min") == 0 ? fmin(x, y) : fmax(x, y));
            if (!isfinite(result_number)) report_error(expr->line, "math function produced a non-finite result");
            else { Value result = { .kind = V_NUMBER, .as.number = result_number }; free(args); return result; }
        } else if (strcmp(name, "memread") == 0 && expr->as.call.count == 2) {
            double index;
            if (args[0].kind != V_MEMORY || !number_value(args[1], &index, expr->line)) report_error(expr->line, "memread expects (memory, index)");
            else if (index < 0 || index >= (double)args[0].as.memory->size || floor(index) != index) report_error(expr->line, "memory index out of bounds");
            else {
                unsigned char byte;
                memory_read(args[0].as.memory, (size_t)index, &byte);
                Value result = { .kind = V_NUMBER, .as.number = byte };
                free(args); return result;
            }
        } else if (strcmp(name, "memwrite") == 0 && expr->as.call.count == 3) {
            double index, byte;
            if (args[0].kind != V_MEMORY || !number_value(args[1], &index, expr->line) || !number_value(args[2], &byte, expr->line)) report_error(expr->line, "memwrite expects (memory, index, byte)");
            else if (index < 0 || index >= (double)args[0].as.memory->size || floor(index) != index || byte < 0 || byte > 255 || floor(byte) != byte) report_error(expr->line, "memwrite index or byte is out of range");
            else {
                memory_write(args[0].as.memory, (size_t)index, (unsigned char)byte);
                free(args); return nil_value();
            }
        } else if (strcmp(name, "tostring") == 0 && expr->as.call.count == 1) {
            char buffer[128];
            if (args[0].kind == V_STRING) { Value result = args[0]; free(args); return result; }
            if (args[0].kind == V_NUMBER) snprintf(buffer, sizeof(buffer), "%.15g", args[0].as.number);
            else if (args[0].kind == V_BOOL) snprintf(buffer, sizeof(buffer), "%s", args[0].as.boolean ? "yes" : "no");
            else snprintf(buffer, sizeof(buffer), "void");
            Value result = { .kind = V_STRING, .as.string = copy_range(buffer, strlen(buffer)) }; free(args); return result;
        }
    }
    if (!failed) report_error(expr->line, "attempt to call a non-function value");
    free(args); return nil_value();
}

static Value evaluate(Expr *expr, Env *env) {
    if (!expr || failed) return nil_value();
    if (expr->kind == E_VALUE) return expr->as.value;
    if (expr->kind == E_VARIABLE) {
        Value *value = find_value(env, expr->as.name);
        if (!value) { report_error(expr->line, "use of an undefined variable"); return nil_value(); }
        return *value;
    }
    if (expr->kind == E_CALL) return call_function(expr, env);
    if (expr->kind == E_UNARY) {
        Value right = evaluate(expr->as.unary.right, env);
        if (strcmp(expr->as.unary.op, "invert") == 0) return (Value){ .kind = V_BOOL, .as.boolean = !truthy(right) };
        double number; if (!number_value(right, &number, expr->line)) return nil_value();
        return (Value){ .kind = V_NUMBER, .as.number = -number };
    }
    const char *op = expr->as.binary.op;
    if (strcmp(op, "define=") == 0) {
        if (expr->as.binary.left->kind != E_VARIABLE) {
            report_error(expr->line, "define = expects a variable name on its left");
            return nil_value();
        }
        Value expected = evaluate(expr->as.binary.right, env);
        if (expected.kind != V_BOOL) {
            report_error(expr->line, "define = expects yes or no on its right");
            return nil_value();
        }
        bool is_declared = find_value(env, expr->as.binary.left->as.name) != NULL;
        return (Value){ .kind = V_BOOL, .as.boolean = is_declared == expected.as.boolean };
    }
    Value left = evaluate(expr->as.binary.left, env);
    if (strcmp(op, "both") == 0) return truthy(left) ? evaluate(expr->as.binary.right, env) : left;
    if (strcmp(op, "either") == 0) return truthy(left) ? left : evaluate(expr->as.binary.right, env);
    Value right = evaluate(expr->as.binary.right, env);
    if (strcmp(op, "==") == 0 || strcmp(op, "~=") == 0) {
        bool equal = left.kind == right.kind;
        if (equal && left.kind == V_NUMBER) equal = left.as.number == right.as.number;
        else if (equal && left.kind == V_STRING) equal = strcmp(left.as.string, right.as.string) == 0;
        else if (equal && left.kind == V_BOOL) equal = left.as.boolean == right.as.boolean;
        else if (equal && left.kind == V_MEMORY) equal = left.as.memory == right.as.memory;
        return (Value){ .kind = V_BOOL, .as.boolean = strcmp(op, "==") == 0 ? equal : !equal };
    }
    if (strcmp(op, "..") == 0 && left.kind == V_STRING && right.kind == V_STRING) {
        size_t a_len = strlen(left.as.string), b_len = strlen(right.as.string);
        char *joined = malloc(a_len + b_len + 1);
        if (!joined) { fputs("out of memory\\n", stderr); exit(2); }
        memcpy(joined, left.as.string, a_len);
        memcpy(joined + a_len, right.as.string, b_len + 1);
        return (Value){ .kind = V_STRING, .as.string = joined };
    }
    if (strcmp(op, "+") == 0 && left.kind == V_STRING && right.kind == V_STRING) {
        size_t a = strlen(left.as.string), b = strlen(right.as.string);
        char *joined = malloc(a + b + 1); memcpy(joined, left.as.string, a); memcpy(joined + a, right.as.string, b + 1);
        return (Value){ .kind = V_STRING, .as.string = joined };
    }
    double a, b;
    if (!number_value(left, &a, expr->line) || !number_value(right, &b, expr->line)) return nil_value();
    if (strcmp(op, "<") == 0) return (Value){ .kind = V_BOOL, .as.boolean = a < b };
    if (strcmp(op, "<=") == 0) return (Value){ .kind = V_BOOL, .as.boolean = a <= b };
    if (strcmp(op, ">") == 0) return (Value){ .kind = V_BOOL, .as.boolean = a > b };
    if (strcmp(op, ">=") == 0) return (Value){ .kind = V_BOOL, .as.boolean = a >= b };
    if (strcmp(op, "+") == 0) return (Value){ .kind = V_NUMBER, .as.number = a + b };
    if (strcmp(op, "-") == 0) return (Value){ .kind = V_NUMBER, .as.number = a - b };
    if (strcmp(op, "*") == 0) return (Value){ .kind = V_NUMBER, .as.number = a * b };
    if (strcmp(op, "**") == 0) {
        double result = pow(a, b);
        if (!isfinite(result)) { report_error(expr->line, "exponentiation produced a non-finite result"); return nil_value(); }
        return (Value){ .kind = V_NUMBER, .as.number = result };
    }
    if (strcmp(op, "/") == 0) {
        if (b == 0) { report_error(expr->line, "division by zero"); return nil_value(); }
        return (Value){ .kind = V_NUMBER, .as.number = a / b };
    }
    if (strcmp(op, "//") == 0) {
        if (b == 0) { report_error(expr->line, "division by zero"); return nil_value(); }
        return (Value){ .kind = V_NUMBER, .as.number = floor(a / b) };
    }
    if (strcmp(op, "%") == 0) {
        if (b == 0) { report_error(expr->line, "division by zero"); return nil_value(); }
        return (Value){ .kind = V_NUMBER, .as.number = fmod(a, b) };
    }
    return nil_value();
}

static Result execute(Stmt *stmt, Env *env) {
    if (failed) return (Result){0};
    switch (stmt->kind) {
        case S_LOCAL: define_value(env, stmt->as.variable.name, stmt->as.variable.value ? evaluate(stmt->as.variable.value, env) : nil_value()); break;
        case S_ASSIGN: {
            Value value = evaluate(stmt->as.variable.value, env);
            Value *slot = find_value(env, stmt->as.variable.name);
            if (slot) *slot = value; else define_value(env, stmt->as.variable.name, value);
            break;
        }
        case S_EXPR: (void)evaluate(stmt->as.expression, env); break;
        case S_OUTPUT: write_value(stdout, evaluate(stmt->as.expression, env)); fputc('\n', stdout); break;
        case S_ERROR_OUTPUT: write_value(stderr, evaluate(stmt->as.expression, env)); fputc('\n', stderr); break;
        case S_MEMORY: {
            Value size_value = evaluate(stmt->as.memory.size, env); double size;
            if (!number_value(size_value, &size, stmt->line)) break;
            if (size < 0 || size > 16777216 || floor(size) != size) { report_error(stmt->line, "memory size must be an integer from 0 to 16777216"); break; }
            Memory *memory = memory_create((size_t)size);
            if (!memory) { report_error(stmt->line, "unable to allocate memory buffer"); break; }
            Value value = { .kind = V_MEMORY, .as.memory = memory };
            Value *slot = find_value(env, stmt->as.memory.name);
            if (slot) *slot = value; else define_value(env, stmt->as.memory.name, value);
            break;
        }
        case S_INCLUDE: {
            Value path = evaluate(stmt->as.expression, env);
            if (path.kind != V_STRING) { report_error(stmt->line, "!#libinc expects a quoted path"); break; }
            char *source = read_entire_file(path.as.string, NULL);
            if (source) { run_source(source, path.as.string, env); free(source); }
            else failed = true;
            break;
        }
        case S_EMBEDDED_INCLUDE: {
            const char *path = stmt->as.embedded_library;
            const char *filename = strrchr(path, '/');
            filename = filename ? filename + 1 : path;
            size_t filename_length = strlen(filename);
            if (filename_length <= 3 || strcmp(filename + filename_length - 3, ".yl") != 0) {
                report_error(stmt->line, "embedded imports must name a .yl file");
                break;
            }
            char *library_name = copy_range(filename, filename_length - 3);
            const char *source = y_embedded_library(library_name);
            if (!source) {
                char message[256];
                snprintf(message, sizeof(message), "unknown embedded library '%%%s%%'", path);
                report_error(stmt->line, message);
                free(library_name);
                break;
            }
            run_source(source, library_name, env);
            free(library_name);
            break;
        }
        case S_FUNCTION: {
            Function *function = calloc(1, sizeof(*function));
            function->name = stmt->as.function.name; function->params = stmt->as.function.params;
            function->param_count = stmt->as.function.count; function->body = stmt->as.function.body; function->closure = env;
            Value value = { .kind = V_FUNCTION, .as.function = function };
            define_value(env, function->name, value); break;
        }
        case S_RETURN: return (Result){ .returned = true, .value = stmt->as.returned.value ? evaluate(stmt->as.returned.value, env) : nil_value() };
        case S_BREAK: return (Result){ .broke = true };
        case S_IF: return execute_list(truthy(evaluate(stmt->as.conditional.condition, env)) ? stmt->as.conditional.yes : stmt->as.conditional.no, env);
        case S_WHILE: {
            size_t iterations = 0;
            while (truthy(evaluate(stmt->as.loop.condition, env)) && !failed) {
                if (++iterations > 1000000) { report_error(stmt->line, "loop iteration limit exceeded"); break; }
                Result result = execute_list(stmt->as.loop.body, env);
                if (result.returned) return result;
                if (result.broke) break;
            }
            break;
        }
    }
    return (Result){0};
}

static Result execute_list(Stmt *stmt, Env *env) {
    for (; stmt && !failed; stmt = stmt->next) {
        Result result = execute(stmt, env);
        if (result.returned || result.broke) return result;
    }
    return (Result){0};
}

static bool run_source(const char *source, const char *path, Env *env) {
    const char *old_path = active_path; active_path = path;
    Parser parser = { .path = path };
    tokenize(&parser, source);
    Stmt *program = failed ? NULL : parse_block(&parser, false);
    if (!failed && peek(&parser)->kind != T_EOF) report_error(peek(&parser)->line, "unexpected trailing input");
    if (!failed) execute_list(program, env);
    active_path = old_path;
    return !failed;
}

int y_program_main(int argc, char **argv) {
    Env *globals = new_env(NULL);
    if (argc == 2 && strcmp(argv[1], "--version") == 0) {
        printf("Y_ %s\n", Y_VERSION);
        return 0;
    }
    if (argc > 2) { fprintf(stderr, "usage: %s [--version | script.yc]\n", argv[0]); return 2; }
    if (argc == 2) {
        char *source = read_entire_file(argv[1], NULL);
        if (!source) return 1;
        run_source(source, argv[1], globals); free(source);
        return failed ? 1 : 0;
    }
    char line[4096];
    fputs("Y_ interactive mode (Ctrl-D to exit)\n", stdout);
    while (fputs("y_> ", stdout), fflush(stdout), fgets(line, sizeof(line), stdin)) {
        failed = false;
        if (!run_source(line, "<repl>", globals)) failed = false;
    }
    return 0;
}