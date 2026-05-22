#ifndef _HSH_H_
#define _HSH_H_

#define HSH_READ_BUF_SIZE 1024

#include "lexer.h"

enum NodeType
{
    NODE_CMD,
    NODE_PIPE,
    NODE_AND,
    NODE_OR,
    NODE_SEQ
};
typedef enum NodeType NodeType;

struct Redir
{
    TokenType direction;
    char *filename;
};
typedef struct Redir Redir;

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

struct Parser
{
    Token *tokens;
    int pos;
};
typedef struct Parser Parser;

/*
    Function declarations.
*/
char *read_line(void); // Read a line of input from the user.

#endif
