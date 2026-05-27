#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <setjmp.h>
#include "hsh.h"

static void parser_error(Parser *p, const char *msg);
static TokenType peek(Parser *p);
static Token advance(Parser *p);
static Token take_token(Parser *p);
static Token consume(Parser *p, TokenType expected);
static void ensure_argv_capacity(char ***argv, int *argc, int *capacity);
static void ensure_redir_capacity(Redir **redirs, int *redir_count, int *capacity);
static Node *parse_cmd(Parser *p);
static Node *parse_pipeline(Parser *p);
static Node *parse_and_or(Parser *p);
static Node *parse_sequence(Parser *p);
static void free_tokens_remaining(Token *tokens);
static void free_cmd_parts(Parser *p);

/*
    Reports a parsing error, frees any allocated resources, and jumps back to the main loop.
*/
static void parser_error(Parser *p, const char *msg)
{
    fprintf(stderr, "hsh: %s\n", msg);
    free_cmd_parts(p);
    /*
        Free any partially constructed AST nodes.
    */
    if (p->partial_root != NULL)
    {
        free_tree(p->partial_root);
        p->partial_root = NULL;
    }
    free_tokens_remaining(p->tokens);
    longjmp(err_jmp, ERR_PARSE_FAILED);
}

/*
    Just a peek, to make a decision on what to parse next.
    There is no need to consume the token.
*/
static TokenType peek(Parser *p)
{
    return p->tokens[p->pos].type;
}

/*
    Advance the parser to the next token and return the current token.
*/
static Token advance(Parser *p)
{
    Token tmp = p->tokens[p->pos];
    p->pos++;
    return tmp;
}

/*
    Consume a token and return it. This is used when we want to take
    ownership of the token's value and transfer it to the AST node.
    The aim of this function is to avoid repeated free operations.
*/
static Token take_token(Parser *p)
{
    Token tmp = advance(p);
    p->tokens[p->pos - 1].value = NULL; /* ownership moved to AST */
    return tmp;
}

/*
    Consume a token of the expected type, or report a syntax error.
*/
static Token consume(Parser *p, TokenType expected)
{
    if (peek(p) != expected)
    {
        parser_error(p, "Syntax error: unexpected token type.");
    }
    return take_token(p);
}

/*
    Ensure that the argv array has enough capacity to hold additional arguments.
    If not, it doubles the capacity and reallocates the array.
*/
static void ensure_argv_capacity(char ***argv, int *argc, int *capacity)
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

/*
    Ensure that the redirs array has enough capacity to hold additional redirections.
    If not, it doubles the capacity and reallocates the array.
*/
static void ensure_redir_capacity(Redir **redirs, int *redir_count, int *capacity)
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
static Node *parse_cmd(Parser *p)
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
        Set the current command parts in the parser for error handling.
        If a parsing error occurs while parsing the command arguments or redirections,
        we can free these resources in the parser_error function.
    */
    p->cur_argv = argv;
    p->cur_argc = 0;
    p->cur_redirs = redirs;
    p->cur_redir_count = 0;

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
            Token tmp = take_token(p);
            argv[argc] = tmp.value;
            argc++;
            p->cur_argc = argc;
        }

        /*
            Handle the case where the current token is a redirection operator.
        */
        else if (peek(p) == TOKEN_REDIR_IN || peek(p) == TOKEN_REDIR_OUT || peek(p) == TOKEN_REDIR_APPEND)
        {
            ensure_redir_capacity(&redirs, &redir_count, &redir_capacity);
            Redir *redir = &redirs[redir_count];
            redir->dir_type = peek(p);
            take_token(p); // Consume the redirection operator.
            Token filename_token = consume(p, TOKEN_WORD);
            redir->filename = filename_token.value;
            redir_count++;
            p->cur_redir_count = redir_count;
        }
        else
        {
            break;
        }
    }

    /*
        If we haven't parsed any command arguments or redirections, it's a syntax error.
    */
    if (argc == 0 && redir_count == 0)
    {
        parser_error(p, "Syntax error: expected command or redirection.");
    }

    argv[argc] = NULL; // Null-terminate the argv array.

    /*
        Allocate memory for the command node and fill in the details.
        The AST's command node takes ownership of the argv and redirs arrays,
        so we set the parser's current command parts to NULL to avoid double free issues.
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

    p->cur_argv = NULL; // Ownership of argv has been transferred to the AST node.
    p->cur_argc = 0;
    p->cur_redirs = NULL; // Ownership of redirs has been transferred to the AST node.
    p->cur_redir_count = 0;

    return cmd_node;
}

/*
    Parse a pipeline of commands, which consists of one or
    more command nodes connected by pipe operators.
*/
static Node *parse_pipeline(Parser *p)
{
    Node *left = parse_cmd(p);
    p->partial_root = left; // Save the partially constructed AST for cleanup in case of error.
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
        left = node;            // The new node becomes the left operand for the next iteration.
        p->partial_root = left; // Update the partially constructed AST for cleanup in case of error.
    }
    return left;
}

/*
    Parse a sequence of commands connected by '&&' or '||' operators.
*/
static Node *parse_and_or(Parser *p)
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
        left = node;            // The new node becomes the left operand for the next iteration.
        p->partial_root = left; // Update the partially constructed AST for cleanup in case of error.
    }
    return left;
}

/*
    Parse a sequence of commands separated by ';' operators.
*/
static Node *parse_sequence(Parser *p)
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
        left = node;            // The new node becomes the left operand for the next iteration.
        p->partial_root = left; // Update the partially constructed AST for cleanup in case of error.
    }
    return left;
}

/*
    Frees any remaining tokens in the token stream.
*/
static void free_tokens_remaining(Token *tokens)
{
    if (tokens == NULL)
    {
        return;
    }
    for (int i = 0; tokens[i].type != TOKEN_END; i++)
    {
        free(tokens[i].value);
        tokens[i].value = NULL;
    }
}

/*
    Frees the memory allocated for the current command arguments and redirections
    in the parser. This is used to clean up resources in case of a parsing error.
*/
static void free_cmd_parts(Parser *p)
{
    /*
        Free the current command arguments.
    */
    if (p->cur_argv)
    {
        for (int i = 0; i < p->cur_argc; i++)
        {
            free(p->cur_argv[i]);
        }
        free(p->cur_argv);
        p->cur_argv = NULL;
    }

    /*
        Free the current command redirections.
    */
    if (p->cur_redirs)
    {
        for (int i = 0; i < p->cur_redir_count; i++)
        {
            free(p->cur_redirs[i].filename);
        }
        free(p->cur_redirs);
        p->cur_redirs = NULL;
    }
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
        Initialize the current command parts.
    */
    p.cur_argv = NULL;
    p.cur_argc = 0;
    p.cur_redirs = NULL;
    p.cur_redir_count = 0;
    p.partial_root = NULL;

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
        p.partial_root = root; // Save the partially constructed AST for cleanup.
        parser_error(&p, "Syntax error: unexpected token after end of command.");
    }

    return root;
}
