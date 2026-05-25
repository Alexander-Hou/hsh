#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <setjmp.h>
#include "hsh.h"

jmp_buf err_jmp;

int main(void)
{
    while (1)
    {
        if(setjmp(err_jmp)!=0){
            /*
                clean up
            */
            continue;
        }

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
        Node *ast = parse(tokens);
        free(tokens);
        free(cmd);
    }
    return 0;
}
