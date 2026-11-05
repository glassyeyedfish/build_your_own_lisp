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
//      Lisp Value
// ============================================================================

enum { LVAL_NUM, LVAL_ERR };
enum { LERR_DIV_ZERO, LERR_BAD_OP, LERR_BAD_NUM };

typedef struct lval_t {
        int type;
        long num;
        int err;
} lval_t;

lval_t 
lval_num(long num)
{
        lval_t v = {0};
        v.type = LVAL_NUM;
        v.num = num;
        return v;
}

lval_t 
lval_err(int err)
{
        lval_t v = {0};
        v.type = LVAL_ERR;
        v.err = err;
        return v;
}

void 
lval_print(lval_t v)
{
        switch (v.type)
        {
        case LVAL_NUM: 
                printf("%ld", v.num); 
                break;

        case LVAL_ERR:
                switch (v.err) 
                {
                        case LERR_DIV_ZERO: printf("[ERR]: Division by zero!"); break;
                        case LERR_BAD_OP: printf("[ERR]: Invalid operation!"); break;
                        case LERR_BAD_NUM: printf("[ERR]: Invalid number!"); break;
                }
                break;
        }

}

// ============================================================================
//      Evaluation Functions
// ============================================================================

typedef lval_t (*eval_op_fptr_t) (lval_t, lval_t);

lval_t eval_op_add(lval_t a, lval_t b) { return lval_num(a.num + b.num); }
lval_t eval_op_sub(lval_t a, lval_t b) { return lval_num(a.num - b.num); }
lval_t eval_op_mul(lval_t a, lval_t b) { return lval_num(a.num * b.num); }
lval_t eval_op_div(lval_t a, lval_t b) { return (b.num != 0) ? lval_num(a.num / b.num) : lval_err(LERR_DIV_ZERO); }
lval_t eval_op_mod(lval_t a, lval_t b) { return lval_num(a.num % b.num); }
lval_t eval_op_pow(lval_t a, lval_t b) { return lval_num(pow(a.num, b.num)); }
lval_t eval_op_min(lval_t a, lval_t b) { return (a.num < b.num) ? lval_num(a.num) : lval_num(b.num); }
lval_t eval_op_max(lval_t a, lval_t b) { return (a.num > b.num) ? lval_num(a.num) : lval_num(b.num); }

eval_op_fptr_t
eval_op(mpc_ast_t* t)
{
        // op   : '+' | '-' | '*' | '/' | '%' | '^' | \"min\" | \"max\" ;

        if (strcmp(t->contents, "+") == 0) return eval_op_add;
        if (strcmp(t->contents, "-") == 0) return eval_op_sub;
        if (strcmp(t->contents, "*") == 0) return eval_op_mul;
        if (strcmp(t->contents, "/") == 0) return eval_op_div;
        if (strcmp(t->contents, "%") == 0) return eval_op_mod;
        if (strcmp(t->contents, "^") == 0) return eval_op_pow;
        if (strcmp(t->contents, "min") == 0) return eval_op_min;
        if (strcmp(t->contents, "max") == 0) return eval_op_max;

        return NULL;
}

lval_t 
eval_expr(mpc_ast_t *t)
{
        // expr : <num> | '(' <op> <expr>+ ')' ;
        if (strstr(t->tag, "num"))
        {
                return lval_num(atoi(t->contents));
        }

        eval_op_fptr_t op = eval_op(t->children[1]);

        lval_t r = eval_expr(t->children[2]);
        if (r.type == LVAL_ERR) return r;

        for (int i = 3; i < (t->children_num - 1); i += 1)
        {
                lval_t s = eval_expr(t->children[i]);
                if (s.type == LVAL_ERR) return s;
                r = op(r, s);
        }

        return r;
}

lval_t
eval_lisp(mpc_ast_t *t)
{
        // lisp : /^/ <op> <expr>+ /$/ ;
        eval_op_fptr_t op = eval_op(t->children[1]);

        lval_t r = eval_expr(t->children[2]);
        if (r.type == LVAL_ERR) return r;

        for (int i = 3; i < (t->children_num - 1); i += 1)
        {
                lval_t s = eval_expr(t->children[i]);
                if (s.type == LVAL_ERR) return s;
                r = op(r, s);
        }

        return r;
}

// ============================================================================
//      Main Function1
// ============================================================================

int 
main(void)
{
        // Polish notation stuff.
        mpc_parser_t *num_parser = mpc_new("num");
        mpc_parser_t *op_parser = mpc_new("op");
        mpc_parser_t *expr_parser = mpc_new("expr");
        mpc_parser_t *lisp_parser = mpc_new("lisp");

        mpca_lang(MPCA_LANG_DEFAULT,
"\
num  : /-?[0-9]+/ ;\
op   : '+' | '-' | '*' | '/' | '%' | '^' | \"min\" | \"max\" ;\
expr : <num> | '(' <op> <expr>+ ')' ;\
lisp : /^/ <op> <expr>+ /$/ ;\
",
                num_parser,
                op_parser,
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
                char *input_buffer = readline("<- ");
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
                        continue;
                }

                lval_t v = eval_lisp(r.output);

                // PRINT
                printf("-> ");
                lval_print(v);
                printf("\n\n");
                // mpc_ast_print(r.output);

                // LOOP
                mpc_ast_delete(r.output);
                free(input_buffer);
        }

        // Clean up, clean up, everybody clean up!
        mpc_cleanup(4, num_parser, op_parser, expr_parser, lisp_parser);
        return EXIT_SUCCESS;
}