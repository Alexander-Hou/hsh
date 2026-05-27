/*
    hsh.h - Header file for the hsh shell project.
*/

#ifndef HSH_H
#define HSH_H

#define ERR_OUT_OF_MEMORY  1
#define ERR_PARSE_FAILED   2

#include <setjmp.h>
#include "lexer.h"
#include "parser.h"
#include "reader.h"
#include "executor.h"

extern jmp_buf err_jmp;

#endif
