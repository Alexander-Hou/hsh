#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <setjmp.h>
#include "hsh.h"

Node *parse(Token *tokens)
{
    /*
        Initialize the parser with the token stream and
        set the position to the beginning.
    */
    Parser p;
    p.tokens = tokens;
    p.pos = 0;

    /*
        Parse the token stream into an abstract syntax tree.
    */
    Node *root = parse_sequence(&p);

    /*
        After parsing, we should have consumed all tokens. If there are any
        remaining tokens, it's a syntax error.
    */
    if (peek(&p) != TOKEN_END)
    {
        fprintf(stderr, "hsh: syntax error.\n");
        free_tree(root);
        longjmp(err_jmp, ERR_PARSE_FAILED);
    }

    return root;
}

/*
    Just a peek, to make a decision on what to parse next.
    There is no need to consume the token.
*/
TokenType peek(Parser *p)
{
    return p->tokens[p->pos].type;
}

/*
    Advance the parser to the next token and return the current token.
*/
Token advance(Parser *p)
{
    Token tmp = p->tokens[p->pos];
    p->pos++;
    return tmp;
}

/*
    Consume a token of the expected type, or report a syntax error.
*/
Token consume(Parser *p, TokenType expected)
{
    if (peek(p) != expected)
    {
        fprintf(stderr, "hsh: syntax error.\n");
        longjmp(err_jmp, ERR_PARSE_FAILED);
    }
    return advance(p);
}

void ensure_argv_capacity(char ***argv, int *argc, int *capacity)
{
    if (*argc + 1 >= *capacity)
    {
        *capacity *= 2;
        char **new_argv = (char **)realloc(*argv, sizeof(char *) * (*capacity));
        if (new_argv == NULL)
        {
            free(*argv);
            fprintf(stderr, "hsh: out of memory.\n");
            exit(EXIT_FAILURE);
        }
        *argv = new_argv;
    }
}

void ensure_redir_capacity(Redir **redirs, int *redir_count, int *capacity)
{
    if (*redir_count + 1 >= *capacity)
    {
        *capacity *= 2;
        Redir *new_redirs = (Redir *)realloc(*redirs, sizeof(Redir) * (*capacity));
        if (new_redirs == NULL)
        {
            free(*redirs);
            fprintf(stderr, "hsh: out of memory.\n");
            exit(EXIT_FAILURE);
        }
        *redirs = new_redirs;
    }
}

/*
    Parse a command node, which consists of a command name,
    arguments, and optional redirections.
    I think this function is somewhat challenging,
    probably because the data is a bit complex.
*/
Node *parse_cmd(Parser *p)
{
    /*
        Allocate memory for the argv array.
    */
    int argv_capacity = HSH_ARGV_BUF_SIZE;
    char **argv = (char **)malloc(sizeof(char *) * argv_capacity);
    if (argv == NULL)
    {
        fprintf(stderr, "hsh: out of memory.\n");
        exit(EXIT_FAILURE);
    }

    /*
        Allocate memory for the redirs array.
    */
    int redir_capacity = HSH_REDIR_BUF_SIZE;
    Redir *redirs = (Redir *)malloc(sizeof(Redir) * redir_capacity);
    if (redirs == NULL)
    {
        free(argv);
        fprintf(stderr, "hsh: out of memory.\n");
        exit(EXIT_FAILURE);
    }

    /*
        Parse the command arguments and redirections.
    */
    int argc = 0;
    int redir_count = 0;
    while (1)
    {
        /*
            Handle the case where the current token is a word (i.e., a command argument).
        */
        if (peek(p) == TOKEN_WORD)
        {
            ensure_argv_capacity(&argv, &argc, &argv_capacity);
            Token tmp = advance(p);
            argv[argc] = tmp.value;
            argc++;
        }

        /*
            Handle the case where the current token is a redirection operator.
        */
        else if (peek(p) == TOKEN_REDIR_IN || peek(p) == TOKEN_REDIR_OUT || peek(p) == TOKEN_REDIR_APPEND)
        {
            ensure_redir_capacity(&redirs, &redir_count, &redir_capacity);
            Redir *redir = &redirs[redir_count];
            redir->dir_type = peek(p);
            advance(p); // Consume the redirection operator.
            Token filename_token = consume(p, TOKEN_WORD);
            redir->filename = filename_token.value;
            redir_count++;
        }
        else
        {
            break;
        }
    }

    /*
        If we haven't parsed any command arguments or redirections, it's a syntax error.
    */
    if (argc == 0)
    {
        fprintf(stderr, "hsh: syntax error.\n");
        free(argv);
        free(redirs);
        longjmp(err_jmp, ERR_PARSE_FAILED);
    }

    argv[argc] = NULL; // Null-terminate the argv array.

    /*
        Allocate memory for the command node and fill in the details.
    */
    Node *cmd_node = (Node *)malloc(sizeof(Node));
    if (cmd_node == NULL)
    {
        free(argv);
        free(redirs);
        fprintf(stderr, "hsh: out of memory.\n");
        exit(EXIT_FAILURE);
    }
    cmd_node->type = NODE_CMD;
    cmd_node->argv = argv;
    argv = NULL;
    cmd_node->redirs = redirs;
    redirs = NULL;
    cmd_node->redir_count = redir_count;

    return cmd_node;
}

Node *parse_pipeline(Parser *p)
{
    Node *left = parse_cmd(p);
    while (peek(p) == TOKEN_PIPE)
    {
        advance(p); // Consume the pipe operator.
        Node *right = parse_cmd(p);
        Node *node = (Node *)malloc(sizeof(Node));
        if (node == NULL)
        {
            fprintf(stderr, "hsh: out of memory.\n");
            exit(EXIT_FAILURE);
        }
        node->type = NODE_PIPE;
        node->left = left;
        node->right = right;
        left = node; // The new node becomes the left operand for the next iteration.
    }
    return left;
}

Node *parse_and_or(Parser *p)
{
    Node *left = parse_pipeline(p);
    while (peek(p) == TOKEN_AND_IF || peek(p) == TOKEN_OR_IF)
    {
        NodeType type = (peek(p) == TOKEN_AND_IF) ? NODE_AND : NODE_OR;
        advance(p); // Consume the operator.
        Node *right = parse_pipeline(p);
        Node *node = (Node *)malloc(sizeof(Node));
        if (node == NULL)
        {
            fprintf(stderr, "hsh: out of memory.\n");
            exit(EXIT_FAILURE);
        }
        node->type = type;
        node->left = left;
        node->right = right;
        left = node; // The new node becomes the left operand for the next iteration.
    }
    return left;
}

Node *parse_sequence(Parser *p)
{
    Node *left = parse_and_or(p);
    while (peek(p) == TOKEN_SEMICOLON)
    {
        advance(p); // Consume the semicolon.
        if (peek(p) == TOKEN_END)
        {
            break; // Allow trailing semicolon.
        }
        Node *right = parse_and_or(p);
        Node *node = (Node *)malloc(sizeof(Node));
        if (node == NULL)
        {
            fprintf(stderr, "hsh: out of memory.\n");
            exit(EXIT_FAILURE);
        }
        node->type = NODE_SEQ;
        node->left = left;
        node->right = right;
        left = node; // The new node becomes the left operand for the next iteration.
    }
    return left;
}

/*
    Free the memory allocated for the abstract syntax tree.
*/
void free_tree(Node *node)
{
    if (node == NULL)
        return;

    if (node->type == NODE_CMD)
    {
        for (int i = 0; node->argv[i] != NULL; i++)
        {
            free(node->argv[i]);
        }
        free(node->argv);
        for (int i = 0; i < node->redir_count; i++)
        {
            free(node->redirs[i].filename);
        }
        free(node->redirs);
    }
    else
    {
        free_tree(node->left);
        free_tree(node->right);
    }
    free(node);
}
