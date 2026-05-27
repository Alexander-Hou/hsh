#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <setjmp.h>
#include <signal.h>
#include <termios.h>
#include "hsh.h"
#include "signal_handler.h"

jmp_buf err_jmp;

/*
    Function to disable echoing of control characters (like ^C) in the terminal.
    This is done to improve the user experience when they press Ctrl+C to interrupt a command,
    so that it doesn't print ^C on the terminal.
*/
void disable_echoctl(void)
{
    struct termios term;
    if (tcgetattr(STDIN_FILENO, &term) == 0)
    {
        term.c_lflag &= ~ECHOCTL;
        tcsetattr(STDIN_FILENO, TCSANOW, &term);
    }
}

/*
    Cleanup function to free resources allocated during each iteration of the main loop.
    This function is called when an error occurs or when the user interrupts the current command.
*/
static void cleanup_iteration(char *volatile *cmd, Token *volatile *tokens, Node *volatile *ast)
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
    char *volatile cmd = NULL;
    Token *volatile tokens = NULL;
    Node *volatile AST = NULL;

    /*
        Install signal handlers for SIGINT (Ctrl+C) to allow the user to interrupt the current command.
    */
    install_signal_handlers();

    /*
        Disable echoing of control characters (like ^C) in the terminal for a cleaner user experience.
    */
    disable_echoctl();

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
        if (isatty(STDIN_FILENO))
        {
            printf("hsh > ");
            fflush(stdout);
        }

        /*
            Read a line of input from the user.
        */
        cmd = read_line();
        if (cmd == NULL)
        {
            if (feof(stdin))
            {
                break;
            }
            continue;
        }

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
