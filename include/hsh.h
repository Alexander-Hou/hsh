/*
    hsh.h - Header file for the hsh shell project.
    This header file contains the necessary includes and function declarations for the hsh shell project.
    It defines the buffer size for reading input and includes the lexer and parser headers,
    which are essential components of the shell's functionality.
*/

#ifndef _HSH_H_
#define _HSH_H_

#define ERR_OUT_OF_MEMORY  1
#define ERR_PARSE_FAILED   2

#include <setjmp.h>
#include "lexer.h"
#include "parser.h"
#include "reader.h"

extern jmp_buf err_jmp;

#endif
