#ifndef _PARSER_H_
#define _PARSER_H_

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
};
typedef struct Parser Parser;

/*
    Function prototypes for the parser
*/
Node *parse(Token *);
TokenType peek(Parser *);                     // Look at the current token without consuming it.
Token advance(Parser *);                      // Consume the current token and return it.
Token consume(Parser *, TokenType);            // Consume the current token if it matches the expected type, otherwise report an error.
void ensure_argv_capacity(char ***argv, int* argc, int* capacity);  // Ensure that the argv array has enough capacity to hold additional arguments.
void ensure_redir_capacity(Redir** redirs, int* redir_count, int* capacity); // Ensure that the redirs array has enough capacity to hold additional redirections.
Node *parse_cmd(Parser *);            // Parse a command node.
Node *parse_pipeline(Parser *);       // Parse a pipeline of commands.
Node *parse_and_or(Parser *);         // Parse a sequence of commands connected by && or ||.
Node *parse_sequence(Parser *);       // Parse a sequence of commands separated by ';'.

#endif
