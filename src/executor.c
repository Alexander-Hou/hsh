#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <errno.h>
#include "hsh.h"

static void exec_cmd_inplace(Node *node);
static void collect_pipeline_nodes(Node *node, Node ***list, int *count, int *cap);
static int wait_child(pid_t pid);
static int execute_cmd(Node *node);
static int execute_pipeline(Node *node);
static int execute_and_or(Node *node);
static int execute_sequence(Node *node);

/*
    Executes a command node by forking a child process and using execvp to run the command.
    It also handles input/output redirections specified in the command node.
*/
static void exec_cmd_inplace(Node *node)
{
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
            if (dup2(fd, node->redirs[i].dir_type == TOKEN_REDIR_IN ? STDIN_FILENO : STDOUT_FILENO) == -1)
            {
                perror("hsh");
                exit(EXIT_FAILURE);
            }
            close(fd);
        }
    }

    if (node->argv[0] == NULL)
        exit(EXIT_SUCCESS);

    execvp(node->argv[0], node->argv);
    perror("hsh");
    exit(EXIT_FAILURE);
}

/*
    Flatten all the commands in the pipeline in order and collect them into an array.
    For example, 'ls | grep txt | wc -l' will be collected as: 
    'cmds[0] = ls, cmds[1] = grep, cmds[2] = wc'. At this time, 'count = 3'.
*/
static void collect_pipeline_nodes(Node *node, Node ***list, int *count, int *cap)
{
    if (node->type == NODE_PIPE)
    {
        collect_pipeline_nodes(node->left, list, count, cap);
        collect_pipeline_nodes(node->right, list, count, cap);
        return;
    }
    if (node->type != NODE_CMD)
    {
        fprintf(stderr, "hsh: invalid pipeline node.\n");
        exit(EXIT_FAILURE);
    }

    /*
        Ensure the list has enough capacity to hold the new node. If not, double the capacity.
    */
    if (*count >= *cap)
    {
        *cap *= 2;
        Node **tmp = realloc(*list, sizeof(Node *) * (*cap));
        if (tmp == NULL)
        {
            fprintf(stderr, "hsh: out of memory.\n");
            exit(EXIT_FAILURE);
        }
        *list = tmp;
        tmp = NULL;
    }
    (*list)[(*count)] = node;
    (*count)++;
}

/*
    Waits for a child process to finish and returns its exit status.
*/
static int wait_child(pid_t pid)
{
    int status;
    while (waitpid(pid, &status, 0) == -1)
    {
        if (errno == EINTR)
        {
            continue;
        }
        return -1;
    }
    return WIFEXITED(status) ? WEXITSTATUS(status) : -1;
}

/*
    Executes a command node by forking a child process and using execvp to run the command.
    It also handles input/output redirections specified in the command node.
*/
static int execute_cmd(Node *node)
{
    pid_t childPid = fork();
    if (childPid == -1)
    {
        perror("hsh");
        return -1;
    }
    if (childPid == 0)
    {
        exec_cmd_inplace(node);
    }
    return wait_child(childPid);
}

/*
    Executes a pipeline of commands by creating a pipe and forking child processes for each command in the pipeline.
    The output of the left command is connected to the input of the right command using the pipe.
*/
static int execute_pipeline(Node *node)
{   
    /*
        Prepare the container for the command node.
    */
    int cap = HSH_LIST_SIZE;
    int count = 0;
    Node **cmds = malloc(sizeof(Node *) * cap);
    if (cmds == NULL)
    {
        fprintf(stderr, "hsh: out of memory.\n");
        return -1;
    }
    collect_pipeline_nodes(node, &cmds, &count, &cap);
    if (count == 0)
    {
        free(cmds);
        return 0;
    }

    /*
        Prepare the container for the pipes. 
        For a pipeline with N commands, we need N-1 pipes to connect them.
    */
    int (*pipes)[2] = NULL;
    if (count > 1)
    {
        pipes = malloc(sizeof(int[2]) * (count - 1));
        if (pipes == NULL)
        {
            free(cmds);
            fprintf(stderr, "hsh: out of memory.\n");
            return -1;
        }
        /*
            Create the necessary pipes for the pipeline. 
            Each pipe connects the output of one command to the input of the next command.
        */
        for (int i = 0; i < count - 1; i++)
        {
            if (pipe(pipes[i]) == -1)
            {
                perror("hsh");
                /*
                    If pipe creation fails, clean up any previously created pipes
                    and free allocated memory before returning an error.
                */
                for (int k = 0; k < i; k++)
                {
                    close(pipes[k][0]);
                    close(pipes[k][1]);
                }
                free(pipes);
                free(cmds);
                return -1;
            }
        }
    }

    /*
        Prepare the container for child pids. We need to keep track of the pids of the child processes we fork so that we can wait for them later.
    */
    pid_t *pids = malloc(sizeof(pid_t) * count);
    if (pids == NULL)
    {
        free(pipes);
        free(cmds);
        fprintf(stderr, "hsh: out of memory.\n");
        return -1;
    }
    /* 
        Fork child processes for each command in the pipeline and
        set up the necessary input/output redirections using the pipes.
    */
    for (int i = 0; i < count; i++)
    {
        pid_t pid = fork();
        if (pid == -1)
        {
            perror("hsh");
            if (count > 1)
            {
                for (int k = 0; k < count - 1; k++)
                {
                    close(pipes[k][0]);
                    close(pipes[k][1]);
                }
            }
            for (int k = 0; k < i; k++)
            {
                wait_child(pids[k]);
            }
            free(pids);
            free(pipes);
            free(cmds);
            return -1;
        }
        if (pid == 0)
        {
            if (count > 1)
            {
                if (i > 0)
                {   
                    if (dup2(pipes[i - 1][0], STDIN_FILENO) == -1)
                    {
                        perror("hsh");
                        exit(EXIT_FAILURE);
                    }
                }
                if (i < count - 1)
                {
                    if (dup2(pipes[i][1], STDOUT_FILENO) == -1)
                    {
                        perror("hsh");
                        exit(EXIT_FAILURE);
                    }
                }
                for (int k = 0; k < count - 1; k++)
                {
                    close(pipes[k][0]);
                    close(pipes[k][1]);
                }
            }
            exec_cmd_inplace(cmds[i]);
        }
        pids[i] = pid;
    }
    
    /*
        After forking all child processes, close the pipes
        in the parent process since they are no longer needed.
    */
    if (count > 1)
    {
        for (int i = 0; i < count - 1; i++)
        {
            close(pipes[i][0]);
            close(pipes[i][1]);
        }
    }

    int status = 0;
    for (int i = 0; i < count; i++)
    {
        int s = wait_child(pids[i]);
        if (i == count - 1)
            status = s; /* return last command status */
    }

    free(pids);
    free(pipes);
    free(cmds);
    return status;
}

/*
    Executes an AND/OR node by evaluating the left child and then the right child based on the operator.
*/
static int execute_and_or(Node *node)
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
static int execute_sequence(Node *node)
{
    execute(node->left);
    return execute(node->right);
}

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
