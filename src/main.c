#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "hsh.h"

int main(void)
{
    while (1)
    {
        if (write(STDOUT_FILENO, "hsh > ", 7) != 7)
        {
            fprintf(stderr, "hsh: error in write.\n");
            exit(EXIT_FAILURE);
        };
        char *cmd = read_line();
        // printf("Input command: %s\n", cmd);
        Token *tokens = tokenize(cmd);
        if(tokens == NULL)
        {
            free(cmd);
            continue;
        }
        for (int i = 0; tokens[i].type != TOKEN_END; i++)
        {
            printf("Token type: %d, value: %s\n", tokens[i].type, tokens[i].value);
        }
        free(tokens);
        free(cmd);
    }
    return 0;
}
