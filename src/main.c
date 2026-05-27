#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <setjmp.h>
#include "hsh.h"

jmp_buf err_jmp;

static void cleanup_iteration(char **cmd, Token **tokens, Node **ast)
{
    if (*ast != NULL)
    {
        free_tree(*ast);
        *ast = NULL;
    }
    if (*tokens != NULL)
    {
        free(*tokens);
        *tokens = NULL;
    }
    if (*cmd != NULL)
    {
        free(*cmd);
        *cmd = NULL;
    }
}

int main(void)
{
    char *cmd = NULL;
    Token *tokens = NULL;
    Node *AST = NULL;

    while (1)
    {
        if (setjmp(err_jmp) != 0)
        {
            cleanup_iteration(&cmd, &tokens, &AST);
            continue;
        }
        /*
            Print the shell prompt.
        */
        if (write(STDOUT_FILENO, "hsh > ", 6) != 6)
        {
            fprintf(stderr, "hsh: error in write.\n");
            exit(EXIT_FAILURE);
        };

        /*
            Read a line of input from the user.
        */
        cmd = read_line();

        /*
            Lexical analysis: tokenize the input command line.
        */
        tokens = tokenize(cmd);
        if (tokens == NULL)
        {
            free(cmd);
            cmd = NULL;
            continue;
        }

        /*
            Parsical analysis: parse the list of tokens and build the abstract syntax tree.
        */
        AST = parse(tokens);

        /*
            Execute the commands represented by the abstract syntax tree.
        */
        execute(AST);

        cleanup_iteration(&cmd, &tokens, &AST);
    }
    return 0;
}
