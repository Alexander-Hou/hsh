#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include "hsh.h"

/*
    Signal handler for SIGINT (Ctrl+C) to allow the user to interrupt the current command.
    When SIGINT is received, we set a flag to indicate that an interrupt has occurred.
*/
static volatile sig_atomic_t g_interrupted = 0;

static void on_sigint(int signo)
{
    (void)signo;
    g_interrupted = 1;
    write(STDOUT_FILENO, "\n", 1);
}

void install_signal_handlers(void)
{
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = on_sigint;
    sigemptyset(&sa.sa_mask);
    if (sigaction(SIGINT, &sa, NULL) == -1)
    {
        perror("hsh: sigaction");
        exit(EXIT_FAILURE);
    }
}
