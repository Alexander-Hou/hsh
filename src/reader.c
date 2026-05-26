#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <unistd.h>
#include <errno.h>
#include "hsh.h"

char *read_line(void)
{
    char *line = NULL;
    size_t bufsize = 0;
    ssize_t ret;
    /*
        Read a line of input from the user using getline, which automatically
        allocates and resizes the buffer as needed. We need to handle several
        cases here:
        1. If getline returns -1, we need to check if it's due to EOF or an error.
        2. If it's EOF, we should exit gracefully.
        3. If it's an error, we need to check if it's due to an interrupt (EINTR) or an out-of-memory condition (ENOMEM).
        4. For EINTR, we should clear the error and prompt the user again.
        5. For ENOMEM, we should report the error and exit.
    */
    while (1)
    {
        ret = getline(&line, &bufsize, stdin);
        
        if (ret == -1)
        {
            if (feof(stdin))
            {
                free(line);
                if (isatty(STDIN_FILENO))
                {
                    printf("\n");
                }
                exit(EXIT_SUCCESS); 
            }
            
            if (errno == EINTR)
            {
                clearerr(stdin);
                
                free(line);
                line = NULL;
                bufsize = 0;
                
                if (isatty(STDIN_FILENO))
                {
                    printf("\nhsh > ");
                    fflush(stdout);
                }
                continue;
            }

            if (errno == ENOMEM)
            {
                free(line);
                fprintf(stderr, "hsh: fatal error: out of memory.\n");
                exit(EXIT_FAILURE);
            }

            perror("hsh: fatal error reading input");
            free(line);
            exit(EXIT_FAILURE);
        }

        break;
    }

    /*
        Remove the trailing newline character, if present.
    */
    if (ret > 0 && line[ret - 1] == '\n')
    {
        line[ret - 1] = '\0';
    }

    return line;
}
