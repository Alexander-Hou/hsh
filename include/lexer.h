/*
    lexer.h - Header file for the lexical analyzer of the hsh shell project.
    This header file defines the necessary data structures and function declarations for the lexical analyzer of the hsh shell project. 
    It includes the definition of token types, the token structure, and the lexical analysis context structure. 
    Additionally, it declares the main function for tokenizing a command line string and several helper functions for processing different types of tokens.
*/

#ifndef LEXER_H
#define LEXER_H

#define HSH_TOKEN_BUF_SIZE 8
#include <stdbool.h>

/*
    Lexical analysis states.
*/
enum LexState
{
    STATE_NORMAL,
    STATE_IN_SINGLE_QUOTE,
    STATE_IN_DOUBLE_QUOTE,
    STATE_ESCAPE,
    STATE_ESCAPE_IN_DOUBLE_QUOTE
};
typedef enum LexState LexState;

/*
    Token types.
*/
enum TokenType
{
    TOKEN_WORD,
    TOKEN_PIPE,
    TOKEN_REDIR_IN,
    TOKEN_REDIR_OUT,
    TOKEN_REDIR_APPEND,
    TOKEN_AND_IF,
    TOKEN_OR_IF,
    TOKEN_SEMICOLON,
    TOKEN_END
};
typedef enum TokenType TokenType;

/*
    Token structure.
*/
struct Token
{
    TokenType type;
    char *value;
};
typedef struct Token Token;

/*
    To make the code more intuitive,
    I encapsulated some local variables of the lexical analyzer into a structure,
    which makes it easier to pass parameters to some processing functions.
*/
struct LexContext
{
    char *buf;
    Token *tokens;
    int capacity;
    int token_start;
    int count;
    int i;
    int j;
    LexState state;
    bool is_accumulate;
};
typedef struct LexContext LexContext;

/*
    Some functions used by the lexical analyzer are declared here.
*/
Token *tokenize(char *);                  // Lexical analysis function, which takes a command line string as input and returns an array of tokens.

#endif
