#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include <limits.h>
#include <math.h>
#include <string.h>

#include "my_readline.h"

#include "mpc.h"







// ============================================================================
//      Panic
// ============================================================================

#define panic() panic_internal(__FILE__, __LINE__);

void 
panic_internal(char* file, int line)
{
        fprintf(stderr, "[ERR] Panic in %s @ %d\n", file, line);
        exit(EXIT_FAILURE);
}







// ============================================================================
//      Lisp Value Creation + Destruction
// ============================================================================

typedef enum lval_type_t { LVAL_ERR, LVAL_NUM, LVAL_SYM, LVAL_SEXPR, LVAL_QEXPR } lval_type_t;

typedef struct lval_t lval_t;

struct lval_t {
        lval_type_t     type;
        char*           chars;
        long            num;
        int             cell_count;
        lval_t**        cell;
};

lval_t*
lval_create_err(char* err)
{
        lval_t* v = malloc(sizeof(lval_t));
        v->type = LVAL_ERR;
        v->chars = malloc(strlen(err) + 1);
        strcpy(v->chars, err);
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
lval_create_sym(char* sym)
{

        lval_t* v = malloc(sizeof(lval_t));
        v->type = LVAL_SYM;
        v->chars = sym;
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
lval_create_qexpr(void)
{
        lval_t* v = malloc(sizeof(lval_t));
        v->type = LVAL_QEXPR;
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
                free(v->chars);
                break;

        case LVAL_NUM:
        case LVAL_SYM:
                break;

        case LVAL_SEXPR:
        case LVAL_QEXPR:
                if (v->cell_count == 0) break;
                for (int i = 0; i < (v->cell_count); i += 1) lval_destroy(v->cell[i]);
                free(v->cell);
                break;
        }

        free(v);
}







// ============================================================================
//      Lisp Value Modification Functions
// ============================================================================

void
lval_add_cell_to_end(lval_t* v, lval_t* new_cell)
{
        v->cell_count += 1;
        v->cell = realloc(v->cell, sizeof(lval_t*) * v->cell_count);
        v->cell[v->cell_count - 1] = new_cell;
}

void 
lval_remove_cell_from_front(lval_t* v)
{
        v->cell_count -= 1;
        for (int i = 0; i < v->cell_count; i += 1)
        {
                v->cell[i] = v->cell[i+1];
        }
        v->cell = realloc(v->cell, sizeof(lval_t*) * v->cell_count);
}







// ============================================================================
//      Lisp Value Creation from AST
// ============================================================================

lval_t* lval_build_expr_from_ast(mpc_ast_t* t);

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
        return lval_create_sym(t->contents);
}

lval_t*
lval_internal_build_sexpr_or_qexpr(mpc_ast_t* t, lval_t* v)
{
        // If the sexpr contains 0 expression, we're done.
        if (t->children_num <= 2) return v;

        for (int i = 1; i < (t->children_num - 1); i += 1)
        {
                lval_t* new_cell = lval_build_expr_from_ast(t->children[i]);
                if (new_cell == NULL) panic();
                lval_add_cell_to_end(v, new_cell);
        }

        return v;
}

lval_t*
lval_build_sexpr_from_ast(mpc_ast_t* t)
{
        // sexpr : '(' <expr>* ')' ;
        lval_t* v = lval_create_sexpr();
        lval_internal_build_sexpr_or_qexpr(t, v);
        return v;
}

lval_t*
lval_build_qexpr_from_ast(mpc_ast_t* t)
{
        // qexpr : '{' <expr>* '}' ;
        lval_t* v = lval_create_qexpr();
        lval_internal_build_sexpr_or_qexpr(t, v);
        return v;
}

lval_t*
lval_build_expr_from_ast(mpc_ast_t* t)
{
        // expr  : <num> | <sym> | <sexpr> ;
        if (strstr(t->tag, "num")) return lval_build_num_from_ast(t);
        if (strstr(t->tag, "sym")) return lval_build_sym_from_ast(t);
        if (strstr(t->tag, "sexpr")) return lval_build_sexpr_from_ast(t);
        if (strstr(t->tag, "qexpr")) return lval_build_qexpr_from_ast(t);

        // Sometimes the first level of the AST is nested inside an empty 
        // node. To fix this we'll just try to go one level deeper any time
        // we find a expr.
        //
        // This is a dumb solution, but hopefully it works and we just
        // uh... don't worry about it? Haw haw haw...
        return lval_build_expr_from_ast(t->children[0]);
}







// ============================================================================
//      Lisp Value Print
// ============================================================================

void lval_print_expr(lval_t* v);

void 
lval_print_err(lval_t* v)
{
        printf("[ERR]: %s", v->chars);
}

void
lval_print_num(lval_t* v)
{
        printf("%ld", v->num);
}

void
lval_print_sym(lval_t* v)
{
        printf("%s", v->chars);
}

void
lval_print_cells(lval_t* v)
{
        for (int i = 0; i < v->cell_count; i += 1) 
        {
                if (i != 0) printf(" ");
                lval_print_expr(v->cell[i]);
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
                printf("(");
                lval_print_cells(v); 
                printf(")");
                break;

        case LVAL_QEXPR: 
                printf("{");
                lval_print_cells(v); 
                printf("}");
                break;
        }
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
        case LVAL_SYM: 
        case LVAL_SEXPR: 
        case LVAL_QEXPR:
                lval_print_expr(v);
                break;

        }
}







// ============================================================================
//      Lisp Value Evaluator
// ============================================================================

lval_t* lval_eval_expr(lval_t* v);

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
lval_eval_list(lval_t* v)
{
        if (v->cell_count < 2)
        {
                lval_destroy(v);
                return lval_create_err("List must have at least one argument!");
        }

        lval_remove_cell_from_front(v);
        v->type = LVAL_QEXPR;
        return v;
}

lval_t*
lval_eval_sym(lval_t* v)
{
        if (v->cell_count < 2) panic();
        lval_t* r = NULL;

        if (strcmp(v->cell[0]->chars, "+") == 0) r = lval_eval_add(v);
        else if (strcmp(v->cell[0]->chars, "-") == 0) r = lval_eval_sub(v);
        else if (strcmp(v->cell[0]->chars, "*") == 0) r = lval_eval_mul(v);
        else if (strcmp(v->cell[0]->chars, "/") == 0) r = lval_eval_div(v);
        else if (strcmp(v->cell[0]->chars, "list") == 0) r = lval_eval_list(v);

        if (r == NULL) panic();

        return r;
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
                        lval_t* w = lval_create_err(v->cell[i]->chars);
                        lval_destroy(v);
                        return w;
                }
        }

        // Now we should have (sym num ...) which we can eval and return.
        return lval_eval_sym(v);
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

        case LVAL_QEXPR:
                return v;
        }
}







// ============================================================================
//      Main Function
// ============================================================================

int 
main(void)
{
        // Set up readline.
        readline_t* my_rl = my_rl_create("<- ", 80);

        // Set up the parser.
        mpc_parser_t* num_parser = mpc_new("num");
        mpc_parser_t* sym_parser = mpc_new("sym");
        mpc_parser_t* sexpr_parser = mpc_new("sexpr");
        mpc_parser_t* qexpr_parser = mpc_new("qexpr");
        mpc_parser_t* expr_parser = mpc_new("expr");

        mpca_lang(MPCA_LANG_DEFAULT,
"\
num   : /-?[0-9]+/ ;\
sym   : '+' | '-' | '*' | '/' | \"list\" ;\
sexpr : '(' <expr>* ')' ;\
qexpr : '{' <expr>* '}' ;\
expr  : <num> | <sym> | <sexpr> | <qexpr> ;\
",
                num_parser,
                sym_parser,
                sexpr_parser,
                qexpr_parser,
                expr_parser
        );

        // Begin the REPL
        puts("");
        puts("Welcome to your own Lisp!");
        puts("Press CTRL+D to exit.");
        puts("---\n");

        while (true)
        {
                // READ
                char* input_buffer = my_rl_readline(my_rl);
                if (!input_buffer) break;

                // EVALUATE
                mpc_result_t r;
                if (!mpc_parse("<stdin>", input_buffer, expr_parser, &r))
                {
                        printf("-> ");
                        mpc_err_print(r.error);
                        printf("\n");

                        mpc_err_delete(r.error);
                        free(input_buffer);
                        continue;
                }

                lval_t* v = lval_build_expr_from_ast((mpc_ast_t*) r.output);
                v = lval_eval_expr(v);

                // PRINT
                printf("-> ");
                lval_print(v);
                printf("\n");
                // mpc_ast_print(r.output);
                printf("\n");

                lval_destroy(v);
                mpc_ast_delete(r.output);
                free(input_buffer);
        }

        // Clean up, clean up, everybody clean up!
        mpc_cleanup(5, 
                num_parser, 
                sym_parser, 
                sexpr_parser,
                qexpr_parser,
                expr_parser 
        );
        my_rl_destroy(my_rl);

        return EXIT_SUCCESS;
}