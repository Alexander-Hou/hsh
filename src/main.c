#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <setjmp.h>
#include "hsh.h"

jmp_buf err_jmp;

static void print_indent(int depth)
{
    for (int i = 0; i < depth; i++)
    {
        printf("  ");
    }
}

static const char *node_type_name(NodeType type)
{
    switch (type)
    {
    case NODE_CMD:
        return "CMD";
    case NODE_PIPE:
        return "PIPE";
    case NODE_AND:
        return "AND";
    case NODE_OR:
        return "OR";
    case NODE_SEQ:
        return "SEQ";
    default:
        return "UNKNOWN";
    }
}

static const char *redir_type_name(TokenType type)
{
    switch (type)
    {
    case TOKEN_REDIR_IN:
        return "<";
    case TOKEN_REDIR_OUT:
        return ">";
    case TOKEN_REDIR_APPEND:
        return ">>";
    default:
        return "?";
    }
}

static void print_ast_rec(const Node *node, int depth)
{
    if (node == NULL)
    {
        print_indent(depth);
        printf("(null)\n");
        return;
    }

    if (node->type == NODE_CMD)
    {
        print_indent(depth);
        printf("%s", node_type_name(node->type));
        if (node->argv != NULL)
        {
            for (int i = 0; node->argv[i] != NULL; i++)
            {
                printf(" %s", node->argv[i]);
            }
        }
        printf("\n");

        for (int i = 0; i < node->redir_count; i++)
        {
            print_indent(depth + 1);
            printf("REDIR %s %s\n", redir_type_name(node->redirs[i].dir_type), node->redirs[i].filename);
        }
        return;
    }

    print_indent(depth);
    printf("%s\n", node_type_name(node->type));
    print_ast_rec(node->left, depth + 1);
    print_ast_rec(node->right, depth + 1);
}

void test_print_ast(Node *root)
{
    printf("AST:\n");
    print_ast_rec(root, 0);
}

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
        // printf("Input command: %s\n", cmd);

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

        AST = parse(tokens);

        test_print_ast(AST);

        cleanup_iteration(&cmd, &tokens, &AST);
    }
    return 0;
}
