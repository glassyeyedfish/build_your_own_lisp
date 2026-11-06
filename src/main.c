#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include <limits.h>
#include <math.h>
#include <string.h>

#include<readline/readline.h>
#include<readline/history.h>

#include "mpc.h"







// ============================================================================
//      Panic
// ============================================================================

#define panic() main_panic(__FILE__, __LINE__);

void 
main_panic(char* file, int line)
{
        fprintf(stderr, "[ERR] Panic in %s @ %d\n", file, line);
        exit(EXIT_FAILURE);
}







// ============================================================================
//      Lisp Value Creation + Destruction
// ============================================================================

typedef enum lval_type_t { LVAL_ERR, LVAL_NUM, LVAL_SYM, LVAL_SEXPR } lval_type_t;
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
        }

        free(v);
}






// ============================================================================
//      Lisp Value Creation from AST
// ============================================================================

lval_t* lval_build_sexpr_from_ast(mpc_ast_t* t);

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
lval_build_expr_from_ast(mpc_ast_t* t)
{
        // expr  : <num> | <sym> | <sexpr> ;
        if (strstr(t->tag, "num")) return lval_build_num_from_ast(t);
        if (strstr(t->tag, "sym")) return lval_build_sym_from_ast(t);
        if (strstr(t->tag, "sexpr")) return lval_build_sexpr_from_ast(t);

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







// ============================================================================
//      Lisp Value Print
// ============================================================================

void lval_print_sexpr(lval_t* v);

void 
lval_print_err(lval_t* v)
{
        printf("[ERR]: %s", v->err);
}

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
lval_print(lval_t* v)
{
        switch(v->type)
        {
        case LVAL_ERR: 
                lval_print_err(v);
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

        }
}







// ============================================================================
//      Lisp Value Evaluator
// ============================================================================

lval_t* lval_eval_sexpr(lval_t* v);

lval_t*
lval_eval_add(lval_t* v)
{
        if (v->cell[1]->type != LVAL_NUM)
        {
                lval_destroy(v);
                return lval_create_err("Can only add numbers!");
        }
        long r = v->cell[1]->num;

        for (int i = 2; i < v->cell_count; i++)
        {
                if (v->cell[i]->type != LVAL_NUM)
                {
                        lval_destroy(v);
                        return lval_create_err("Can only add numbers!");
                }
                r += v->cell[i]->num;
        }

        lval_destroy(v);
        return lval_create_num(r);
}

lval_t*
lval_eval_sub(lval_t* v)
{
        if (v->cell[1]->type != LVAL_NUM)
        {
                lval_destroy(v);
                return lval_create_err("Can only subtract numbers!");
        }
        long r = v->cell[1]->num;

        if (v->cell_count == 2)
        {
                lval_destroy(v);
                return lval_create_num(0 - r);
        }


        for (int i = 2; i < v->cell_count; i++)
        {
                if (v->cell[i]->type != LVAL_NUM)
                {
                        lval_destroy(v);
                        return lval_create_err("Can only subtract numbers!");
                }
                r -= v->cell[i]->num;
        }

        lval_destroy(v);
        return lval_create_num(r);
}

lval_t*
lval_eval_mul(lval_t* v)
{
        if (v->cell[1]->type != LVAL_NUM)
        {
                lval_destroy(v);
                return lval_create_err("Can only multiply numbers!");
        }
        long r = v->cell[1]->num;

        for (int i = 2; i < v->cell_count; i++)
        {
                if (v->cell[i]->type != LVAL_NUM)
                {
                        lval_destroy(v);
                        return lval_create_err("Can only multiply numbers!");
                }
                r *= v->cell[i]->num;
        }

        lval_destroy(v);
        return lval_create_num(r);
}

lval_t*
lval_eval_div(lval_t* v)
{
        if (v->cell[1]->type != LVAL_NUM)
        {
                lval_destroy(v);
                return lval_create_err("Can only divide numbers!");
        }
        long r = v->cell[1]->num;

        for (int i = 2; i < v->cell_count; i++)
        {
                if (v->cell[i]->type != LVAL_NUM)
                {
                        lval_destroy(v);
                        return lval_create_err("Can only divide numbers!");
                }
                if (v->cell[i]->num == 0) {
                        lval_destroy(v);
                        return lval_create_err("Division by zero!");
                }
                r /= v->cell[i]->num;
        }

        lval_destroy(v);
        return lval_create_num(r);
}

lval_t*
lval_eval_sym(lval_t* v)
{
        if (v->cell_count < 2) panic();
        lval_t* r;

        switch(v->cell[0]->sym)
        {
        case LSYM_ADD:
                r = lval_eval_add(v);
                break;

        case LSYM_SUB:
                r = lval_eval_sub(v);
                break;

        case LSYM_MUL:
                r = lval_eval_mul(v);
                break;

        case LSYM_DIV:
                r = lval_eval_div(v);
                break;

        }

        return r;
}

lval_t*
lval_eval_expr(lval_t* v)
{
        switch(v->type)
        {
        case LVAL_ERR:
                return v;

        case LVAL_NUM:
                return v;

        case LVAL_SYM:
                lval_destroy(v);
                return lval_create_err("Symbols only start s-expressions!");

        case LVAL_SEXPR:
                return lval_eval_sexpr(v);
        }
}

lval_t*
lval_eval_sexpr(lval_t* v)
{
        // Propogate the error.
        if (v->type == LVAL_ERR) return v;

        // Return empty s-expressions.
        if (v->cell_count == 0) return v;

        // Return unit s-expressions if they contain a number.
        if (v->cell_count == 1)
        {
                if (v->cell[0]->type == LVAL_NUM) return v;
                lval_destroy(v);
                return lval_create_err("Unit s-expressions must contain number!");
        }

        // Check if s-expression begins with an operation.
        if (v->cell[0]->type != LVAL_SYM)
        {
                lval_destroy(v);
                return lval_create_err("S-Expression should begin with symbol!");
        }

        // Evaluate the remaining expressions in the s-expression.
        for (int i = 1; i < v->cell_count; i += 1)
        {
                v->cell[i] = lval_eval_expr(v->cell[i]);

                if (v->cell[i]->type == LVAL_ERR)
                {
                        lval_t* w = lval_create_err(v->cell[i]->err);
                        lval_destroy(v);
                        return w;
                }
        }

        // Now we should have (sym num ...) which we can eval and return.
        return lval_eval_sym(v);
}







// ============================================================================
//      Main Function
// ============================================================================

int 
main(void)
{
        // Set up the parser.
        mpc_parser_t* num_parser = mpc_new("num");
        mpc_parser_t* sym_parser = mpc_new("sym");
        mpc_parser_t* expr_parser = mpc_new("expr");
        mpc_parser_t* sexpr_parser = mpc_new("sexpr");

        mpca_lang(MPCA_LANG_DEFAULT,
"\
num   : /-?[0-9]+/ ;\
sym   : '+' | '-' | '*' | '/' ;\
expr  : <num> | <sym> | <sexpr> ;\
sexpr : '(' <expr>* ')' ;\
",
                num_parser,
                sym_parser,
                expr_parser,
                sexpr_parser
        );

        // Begin the REPL
        puts("");
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
                if (!mpc_parse("<stdin>", input_buffer, sexpr_parser, &r))
                {
                        printf("-> ");
                        mpc_err_print(r.error);
                        printf("\n");

                        mpc_err_delete(r.error);
                        free(input_buffer);
                        continue;
                }

                lval_t* v = lval_build_sexpr_from_ast(r.output);
                v = lval_eval_sexpr(v);

                // PRINT
                printf("-> ");
                lval_print(v);
                printf("\n\n");
                // mpc_ast_print(r.output);

                lval_destroy(v);
                mpc_ast_delete(r.output);
                free(input_buffer);
        }

        // Clean up, clean up, everybody clean up!
        mpc_cleanup(4, 
                num_parser, 
                sym_parser, 
                expr_parser, 
                sexpr_parser
        );
        return EXIT_SUCCESS;
}