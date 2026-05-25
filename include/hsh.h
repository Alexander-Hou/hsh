/*
    hsh.h - Header file for the hsh shell project.
    This header file contains the necessary includes and function declarations for the hsh shell project.
    It defines the buffer size for reading input and includes the lexer and parser headers,
    which are essential components of the shell's functionality.
*/

#ifndef _HSH_H_
#define _HSH_H_

#define HSH_READ_BUF_SIZE 1024

#define ERR_OUT_OF_MEMORY  1
#define ERR_PARSE_FAILED   2

#include <setjmp.h>
#include "lexer.h"
#include "parser.h"

extern jmp_buf err_jmp;

/*
    Function declarations.
*/
char *read_line(void); // Read a line of input from the user.

#endif
