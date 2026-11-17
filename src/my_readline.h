// ============================================================================
//      
//      my_readline.h
//      A basic library to read input from the console.
//
// ============================================================================

#ifndef MY_READLINE_H
#define MY_READLINE_H

#define MY_READLINE_HISTORY_CAP_INIT 256

typedef struct readline_t {
        char*   prompt;
        int     buf_size;
        int     history_len;
        int     history_cap;
        char**  history;
} readline_t;

readline_t* my_rl_create(char* prompt, int buf_size);
void my_rl_destroy(readline_t* r);
char* my_rl_readline(readline_t* r);

#endif