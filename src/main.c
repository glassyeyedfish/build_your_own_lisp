#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include <limits.h>
#include <math.h>
#include <string.h>

#include<readline/readline.h>
#include<readline/history.h>

#include "mpc.h"

#define panic() fprintf(stderr, "[PANIC] in %s @ %u\n", __FILE__, __LINE__); exit(EXIT_FAILURE);

// ============================================================================
//      Lisp Value Creation + Destruction
// ============================================================================

typedef enum lval_type_t { LVAL_ERR, LVAL_NUM, LVAL_SYM, LVAL_SEXPR, LVAL_LISP } lval_type_t;
typedef enum lval_sym_t { LSYM_ADD, LSYM_SUB, LSYM_MUL, LSYM_DIV } lval_sym_t;

typedef struct lval_t lval_t;

struct lval_t {
        lval_type_t     type;
        char*           err;
        long            num;
        lval_sym_t      sym;
        int             cell_count;
        lval_t**        cell;
};

lval_t*
lval_create_err(char* err)
{
        lval_t* v = malloc(sizeof(lval_t));
        v->type = LVAL_ERR;
        v->err = malloc(strlen(err) + 1);
        strcpy(v->err, err);
        return v;
}

lval_t*
lval_create_num(long num)
{
        lval_t* v = malloc(sizeof(lval_t));
        v->type = LVAL_NUM;
        v->num = num;
        return v;
}

lval_t*
lval_create_sym(lval_sym_t sym)
{

        lval_t* v = malloc(sizeof(lval_t));
        v->type = LVAL_SYM;
        v->sym = sym;
        return v;
}

lval_t*
lval_create_sexpr(void)
{
        lval_t* v = malloc(sizeof(lval_t));
        v->type = LVAL_SEXPR;
        v->cell_count = 0;
        v->cell = NULL;
        return v;
}

lval_t*
lval_create_lisp(void)
{
        lval_t* v = malloc(sizeof(lval_t));
        v->type = LVAL_LISP;
        v->cell_count = 0;
        v->cell = NULL;
        return v;
}

void
lval_destroy(lval_t* v)
{
        switch (v->type)
        {
        case LVAL_ERR:
                free(v->err);
                break;

        case LVAL_NUM:
                break;

        case LVAL_SYM:
                break;

        case LVAL_SEXPR:
                if (v->cell_count == 0) break;
                for (int i = 0; i < (v->cell_count); i += 1) lval_destroy(v->cell[i]);
                free(v->cell);
                break;

        case LVAL_LISP:
                if (v->cell_count == 0) break;
                for (int i = 0; i < (v->cell_count); i += 1) lval_destroy(v->cell[i]);
                free(v->cell);
                break;
        }

        free(v);
}

// ============================================================================
//      Lisp Value Creation from AST
// ============================================================================

lval_t* lval_build_expr_from_ast(mpc_ast_t* t);

void
lval_add_cell(lval_t* v, lval_t* new_cell)
{
        v->cell_count += 1;
        v->cell = realloc(v->cell, sizeof(lval_t*) * v->cell_count);
        v->cell[v->cell_count - 1] = new_cell;
}

lval_t*
lval_build_num_from_ast(mpc_ast_t* t)
{
        // num   : /-?[0-9]+/ ;
        lval_t* v = lval_create_num(atol(t->contents));
        return v;
}

lval_t*
lval_build_sym_from_ast(mpc_ast_t* t)
{
        // sym   : '+' | '-' | '*' | '/' ;
        if (strcmp(t->contents, "+") == 0) return lval_create_sym(LSYM_ADD);
        if (strcmp(t->contents, "-") == 0) return lval_create_sym(LSYM_SUB);
        if (strcmp(t->contents, "*") == 0) return lval_create_sym(LSYM_MUL);
        if (strcmp(t->contents, "/") == 0) return lval_create_sym(LSYM_DIV);

        panic();
        return NULL;
}

lval_t*
lval_build_sexpr_from_ast(mpc_ast_t* t)
{
        // sexpr : '(' <expr>* ')' ;
        lval_t* v = lval_create_sexpr();

        // If the sexpr contains 0 expression, we're done.
        if (t->children_num <= 2) return v;

        for (int i = 1; i < (t->children_num - 1); i += 1)
        {
                lval_t* new_cell = lval_build_expr_from_ast(t->children[i]);
                if (new_cell == NULL) panic();
                lval_add_cell(v, new_cell);
        }

        return v;
}

lval_t*
lval_build_expr_from_ast(mpc_ast_t* t)
{
        // expr  : <num> | <sym> | <sexpr> ;
        if (strstr(t->tag, "num")) return lval_build_num_from_ast(t);
        if (strstr(t->tag, "sym")) return lval_build_sym_from_ast(t);
        if (strstr(t->tag, "sexpr")) return lval_build_sexpr_from_ast(t);

        return NULL;
}

lval_t*
lval_build_lisp_from_ast(mpc_ast_t* t)
{
        // lisp  : /^/ <expr>* /$/ ;
        lval_t* v = lval_create_lisp();

        // If the lisp contains 0 expression, we're done.
        if (t->children_num <= 2) return v;

        for (int i = 1; i < (t->children_num - 1); i += 1)
        {
                lval_t* new_cell = lval_build_expr_from_ast(t->children[i]);
                if (new_cell == NULL) panic();
                lval_add_cell(v, new_cell);
        }

        return v;
}

// ============================================================================
//      Lisp Value Print
// ============================================================================

void lval_print_expr(lval_t* v);

void
lval_print_num(lval_t* v)
{
        printf("%ld", v->num);
}

void
lval_print_sym(lval_t* v)
{
        switch(v->sym)
        {
        case LSYM_ADD:
                printf("+");
                break;

        case LSYM_SUB:
                printf("-");
                break;

        case LSYM_MUL:
                printf("*");
                break;

        case LSYM_DIV:
                printf("/");
                break;

        }
}

void
lval_print_sexpr(lval_t* v)
{
        printf("(");
        for (int i = 0; i < v->cell_count; i += 1) 
        {
                if (i != 0) printf(" ");
                lval_print_expr(v->cell[i]);
        }
        printf(")");
}

void
lval_print_expr(lval_t* v)
{
        switch (v->type)
        {
        case LVAL_ERR:
                panic();
                break;

        case LVAL_NUM: 
                lval_print_num(v); 
                break;

        case LVAL_SYM: 
                lval_print_sym(v); 
                break;

        case LVAL_SEXPR: 
                lval_print_sexpr(v); 
                break;

        case LVAL_LISP:
                panic();
                break;
        }
}

void 
lval_print_lisp(lval_t* v)
{
        for (int i = 0; i < v->cell_count; i += 1) 
        {
                if (i != 0) printf(" ");
                lval_print_expr(v->cell[i]);
        }
}

// ============================================================================
//      Evaluation Functions
// ============================================================================

// typedef lval_t (*eval_op_fptr_t) (lval_t, lval_t);

// lval_t eval_op_add(lval_t a, lval_t b) { return lval_num(a.num + b.num); }
// lval_t eval_op_sub(lval_t a, lval_t b) { return lval_num(a.num - b.num); }
// lval_t eval_op_mul(lval_t a, lval_t b) { return lval_num(a.num * b.num); }
// lval_t eval_op_div(lval_t a, lval_t b) { return (b.num != 0) ? lval_num(a.num / b.num) : lval_err(LERR_DIV_ZERO); }
// lval_t eval_op_mod(lval_t a, lval_t b) { return lval_num(a.num % b.num); }
// lval_t eval_op_pow(lval_t a, lval_t b) { return lval_num(pow(a.num, b.num)); }
// lval_t eval_op_min(lval_t a, lval_t b) { return (a.num < b.num) ? lval_num(a.num) : lval_num(b.num); }
// lval_t eval_op_max(lval_t a, lval_t b) { return (a.num > b.num) ? lval_num(a.num) : lval_num(b.num); }

// eval_op_fptr_t
// eval_op(mpc_ast_t* t)
// {
//         // op   : '+' | '-' | '*' | '/' | '%' | '^' | \"min\" | \"max\" ;

//         if (strcmp(t->contents, "+") == 0) return eval_op_add;
//         if (strcmp(t->contents, "-") == 0) return eval_op_sub;
//         if (strcmp(t->contents, "*") == 0) return eval_op_mul;
//         if (strcmp(t->contents, "/") == 0) return eval_op_div;
//         if (strcmp(t->contents, "%") == 0) return eval_op_mod;
//         if (strcmp(t->contents, "^") == 0) return eval_op_pow;
//         if (strcmp(t->contents, "min") == 0) return eval_op_min;
//         if (strcmp(t->contents, "max") == 0) return eval_op_max;

//         return NULL;
// }

// lval_t 
// eval_expr(mpc_ast_t *t)
// {
//         // expr : <num> | '(' <op> <expr>+ ')' ;
//         if (strstr(t->tag, "num"))
//         {
//                 return lval_num(atoi(t->contents));
//         }

//         eval_op_fptr_t op = eval_op(t->children[1]);

//         lval_t r = eval_expr(t->children[2]);
//         if (r.type == LVAL_ERR) return r;

//         for (int i = 3; i < (t->children_num - 1); i += 1)
//         {
//                 lval_t s = eval_expr(t->children[i]);
//                 if (s.type == LVAL_ERR) return s;
//                 r = op(r, s);
//         }

//         return r;
// }

// lval_t
// eval_lisp(mpc_ast_t *t)
// {
//         // lisp : /^/ <op> <expr>+ /$/ ;
//         eval_op_fptr_t op = eval_op(t->children[1]);

//         lval_t r = eval_expr(t->children[2]);
//         if (r.type == LVAL_ERR) return r;

//         for (int i = 3; i < (t->children_num - 1); i += 1)
//         {
//                 lval_t s = eval_expr(t->children[i]);
//                 if (s.type == LVAL_ERR) return s;
//                 r = op(r, s);
//         }

//         return r;
// }

// ============================================================================
//      Main Function
// ============================================================================

int 
main(void)
{
        // Set up the parser.
        mpc_parser_t* num_parser = mpc_new("num");
        mpc_parser_t* sym_parser = mpc_new("sym");
        mpc_parser_t* sexpr_parser = mpc_new("sexpr");
        mpc_parser_t* expr_parser = mpc_new("expr");
        mpc_parser_t* lisp_parser = mpc_new("lisp");

        mpca_lang(MPCA_LANG_DEFAULT,
"\
num   : /-?[0-9]+/ ;\
sym   : '+' | '-' | '*' | '/' ;\
sexpr : '(' <expr>* ')' ;\
expr  : <num> | <sym> | <sexpr> ;\
lisp  : /^/ <expr>* /$/ ;\
",
                num_parser,
                sym_parser,
                sexpr_parser,
                expr_parser,
                lisp_parser
        );

        // Begin the REPL
        puts("Welcome to your own Lisp!");
        puts("Press CTRL+D to exit.");
        puts("---\n");

        while (true)
        {
                // READ
                char* input_buffer = readline("<- ");
                if (!input_buffer) break;
                add_history(input_buffer);

                // EVALUATE
                mpc_result_t r;
                if (!mpc_parse("<stdin>", input_buffer, lisp_parser, &r))
                {
                        printf("-> ");
                        mpc_err_print(r.error);
                        printf("\n");

                        mpc_err_delete(r.error);
                        free(input_buffer);
                        continue;
                }

                lval_t* v = lval_build_lisp_from_ast(r.output);

                // PRINT
                printf("-> ");
                lval_print_lisp(v);
                printf("\n\n");
                mpc_ast_print(r.output);

                mpc_ast_delete(r.output);
                free(input_buffer);
        }

        // Clean up, clean up, everybody clean up!
        mpc_cleanup(5, num_parser, sym_parser, sexpr_parser, expr_parser, lisp_parser);
        return EXIT_SUCCESS;
}