#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include "hsh.h"

char *read_line(void)
{
    char *line = NULL;
    size_t bufsize = 0;
    ssize_t ret;

    /*
        Read a line of input from the user,
        handling interrupts and end-of-file conditions.
    */
    while (1)
    {
        ret = getline(&line, &bufsize, stdin);

        if (ret == -1)
        {
            if (feof(stdin)) // End of file (Ctrl+D)
            {
                free(line);
                if (isatty(STDIN_FILENO))
                {
                    printf("\n");
                }
                return NULL;
            }

            if (errno == EINTR) // Interrupted by signal (e.g., Ctrl+C)
            {
                clearerr(stdin);

                free(line);
                line = NULL;
                bufsize = 0;

                return NULL;
            }

            if (errno == ENOMEM) // Out of memory
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
