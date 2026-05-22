#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <unistd.h>
#include <errno.h>
#include "hsh.h"

char *read_line(void)
{
    int cur_unit_capacity = HSH_READ_BUF_SIZE;
    char *buf = (char *)malloc(cur_unit_capacity * sizeof(char));
    if (buf == NULL)
    {
        fprintf(stderr, "hsh: out of memory.\n");
        exit(EXIT_FAILURE);
    }

    int i = 0;
    char c;
    while (1)
    {
        int ret = read(STDIN_FILENO, &c, 1);
        if (ret == -1)
        {
            perror("hsh");
            exit(EXIT_FAILURE);
        }
        if (ret == 0)
        {
            fprintf(stderr, "hsh: exit.\n");
            exit(EXIT_FAILURE);
        }
        if (i + 1 >= cur_unit_capacity)
        {
            cur_unit_capacity *= 2;
            char *tmp = (char *)realloc(buf, cur_unit_capacity * sizeof(char));
            if (tmp == NULL)
            {
                free(buf);
                fprintf(stderr, "hsh: out of memory.\n");
                exit(EXIT_FAILURE);
            }
            buf = tmp;
            tmp = NULL;
        }
        *(buf + i) = c;
        i++;
        if (c == '\n')
        {
            buf[i - 1] = '\0';
            break;
        }
    }
    return buf;
}
