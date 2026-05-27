#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>
#include "hsh.h"

/*
    Executes the abstract syntax tree representing a command or a sequence of commands.
*/
int execute(Node *ast)
{
    if (ast == NULL)
        return 0;

    switch (ast->type)
    {
    case NODE_CMD:
        return execute_cmd(ast);
    case NODE_PIPE:
        return execute_pipeline(ast);
    case NODE_AND:
        return execute_and_or(ast);
    case NODE_OR:
        return execute_and_or(ast);
    case NODE_SEQ:
        return execute_sequence(ast);
    default:
        fprintf(stderr, "hsh: unknown AST node type.\n");
        return -1;
    }
}

/*
    Executes a command node by forking a child process and using execvp to run the command.
    It also handles input/output redirections specified in the command node.
*/
int execute_cmd(Node *node)
{
    int childPid = fork();
    switch (childPid)
    {
    case -1:
        perror("hsh");
        return -1;
    case 0:
        if (node->redir_count > 0)
        {
            for (int i = 0; i < node->redir_count; i++)
            {
                int fd;
                switch (node->redirs[i].dir_type)
                {
                case TOKEN_REDIR_OUT:
                    fd = open(node->redirs[i].filename, O_WRONLY | O_CREAT | O_TRUNC, 0644);
                    break;
                case TOKEN_REDIR_APPEND:
                    fd = open(node->redirs[i].filename, O_WRONLY | O_CREAT | O_APPEND, 0644);
                    break;
                case TOKEN_REDIR_IN:
                    fd = open(node->redirs[i].filename, O_RDONLY);
                    break;
                default:
                    fprintf(stderr, "hsh: unknown redirection type.\n");
                    exit(EXIT_FAILURE);
                }
                if (fd == -1)
                {
                    perror("hsh");
                    exit(EXIT_FAILURE);
                }
                /*
                    Redirect the file descriptor to the appropriate standard input/output.
                */
                if (dup2(fd, node->redirs[i].dir_type == TOKEN_REDIR_IN ? STDIN_FILENO : STDOUT_FILENO) == -1)
                {
                    perror("hsh");
                    exit(EXIT_FAILURE);
                }
                close(fd);
            }
        }

        /*
            Execute the command using execvp.
        */
        if (execvp(node->argv[0], node->argv))
        {
            perror("hsh");
            exit(EXIT_FAILURE);
        }
        break;
    default:
        int status;
        waitpid(childPid, &status, 0);
        return WIFEXITED(status) ? WEXITSTATUS(status) : -1; // Return the exit status of the child process
    }
    return 0;
}

/*
    Executes a pipeline of commands by creating a pipe and forking child processes for each command in the pipeline.
    The output of the left command is connected to the input of the right command using the pipe.
*/
int execute_pipeline(Node *node)
{
    int pipefd[2];
    if (pipe(pipefd) == -1)
    {
        perror("hsh");
        return -1;
    }

    int leftPid = fork();
    if (leftPid == -1)
    {
        perror("hsh");
        close(pipefd[0]);
        close(pipefd[1]);
        return -1;
    }
    if (leftPid == 0)
    {
        close(pipefd[0]);
        int ret = dup2(pipefd[1], STDOUT_FILENO);
        if (ret == -1)
        {
            perror("hsh");
            exit(EXIT_FAILURE);
        }
        close(pipefd[1]);
        exit(execute(node->left));
    }

    int rightPid = fork();
    if (rightPid == -1)
    {
        perror("hsh");
        close(pipefd[0]);
        close(pipefd[1]);
        return -1;
    }
    if (rightPid == 0)
    {
        close(pipefd[1]);
        int ret = dup2(pipefd[0], STDIN_FILENO);
        if (ret == -1)
        {
            perror("hsh");
            exit(EXIT_FAILURE);
        }
        close(pipefd[0]);
        exit(execute(node->right));
    }

    close(pipefd[0]);
    close(pipefd[1]);

    int status;
    waitpid(leftPid, &status, 0);
    waitpid(rightPid, &status, 0);
    return WIFEXITED(status) ? WEXITSTATUS(status) : -1; // Return the exit status of the child process
}

/*
    Executes an AND/OR node by evaluating the left child and then the right child based on the operator.
*/
int execute_and_or(Node *node)
{
    int status = execute(node->left);
    if (node->type == NODE_AND)
    {
        return (status == 0) ? execute(node->right) : status;
    }
    if (node->type == NODE_OR)
    {
        return (status != 0) ? execute(node->right) : status;
    }
    return -1;
}

/*
    Executes a sequence of commands by executing the left command and then the right command.
*/
int execute_sequence(Node *node)
{
    execute(node->left);
    return execute(node->right);
}
