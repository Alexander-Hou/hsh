/*
    parser.h - Header file for the parser component of the shell

    This file defines the data structures and function prototypes for the parser,
    which takes a list of tokens produced by the lexer and constructs an abstract
    syntax tree (AST) representing the structure of the command line input.

    The parser supports commands, pipelines, logical operators (&&, ||), and
    command sequences separated by semicolons. It also handles command arguments
    and redirections.
*/

#ifndef PARSER_H
#define PARSER_H

#define HSH_ARGV_BUF_SIZE 4
#define HSH_REDIR_BUF_SIZE 2

#include "lexer.h"
#include "hsh.h"

/*
    Node types for the abstract syntax tree
*/
enum NodeType
{
    NODE_CMD,
    NODE_PIPE,
    NODE_AND,
    NODE_OR,
    NODE_SEQ
};
typedef enum NodeType NodeType;

/*
    Structure representing a redirection in the abstract syntax tree
*/
struct Redir
{
    TokenType dir_type;
    char *filename;
};
typedef struct Redir Redir;

/*
    Structure representing a node in the abstract syntax tree
*/
struct Node
{
    NodeType type;
    union
    {
        struct
        {
            struct Node *left;
            struct Node *right;
        };
        struct
        {
            char **argv;
            Redir *redirs;
            int redir_count;
        };
    };
};
typedef struct Node Node;

/*
    Structure representing the parser
*/
struct Parser
{
    Token *tokens;
    int pos;
    /*
        Extended structure：current command arguments and redirections.
    */
    char **cur_argv;
    int cur_argc;
    Redir *cur_redirs;
    int cur_redir_count;
    Node *partial_root;
};
typedef struct Parser Parser;

/*
    Function prototypes for the parser
*/
Node *parse(Token *);                                      // Parse the list of tokens and return the root of the abstract syntax tree.
void free_tree(Node *);                                    // Free the memory allocated for the abstract syntax tree.

#endif
