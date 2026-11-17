// ============================================================================
//      
//      my_readline.c
//      A basic library to read input from the console.
//
// ============================================================================

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "my_readline.h"

readline_t* 
my_rl_create(char* prompt, int buf_size)
{
        readline_t* r = malloc(sizeof(readline_t));

        r->prompt = malloc(sizeof(char) * strlen(prompt));
        strcpy(r->prompt, prompt);

        r->buf_size = buf_size;

        r->history_len = 0;
        r->history_cap = MY_READLINE_HISTORY_CAP_INIT;

        r->history = malloc(sizeof(char) * r->buf_size * r->history_cap);

        return r;
}

void 
my_rl_destroy(readline_t* r)
{
        free(r->history);
        free(r->prompt);
        free(r);
}

char* 
my_rl_readline(readline_t* r)
{
        if (r->history_len >= r->history_cap)
        {
                r->history_cap += MY_READLINE_HISTORY_CAP_INIT;
                r->history = realloc(r->history, sizeof(char) * r->buf_size * r->history_cap);
        }

        char* s;
        fputs(r->prompt, stdout);
        s = fgets(r->history[r->history_len * r->buf_size], r->buf_size, stdin);
        r->history_len += 1;

        return s;
}