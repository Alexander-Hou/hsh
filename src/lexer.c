#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include "hsh.h"

Token *tokenize(char *buf)
{
    /*
        Initialize the LexContext structure.
    */
    LexContext ctx;
    ctx.buf = buf;
    ctx.capacity = HSH_TOKEN_BUF_SIZE;
    ctx.count = 0;
    ctx.i = 0;
    ctx.is_accumulate = false;
    ctx.state = STATE_NORMAL;
    ctx.tokens = (Token *)malloc(ctx.capacity * sizeof(Token));
    if (ctx.tokens == NULL)
    {
        fprintf(stderr, "hsh: out of memory.\n");
        exit(EXIT_FAILURE);
    }

    /*
        Main lexical analysis loop.
    */
    while (1)
    {
        if (ctx.state == STATE_IN_SINGLE_QUOTE)
        {
            if (ctx.buf[ctx.i] == '\'')
            {
                ctx.state = STATE_NORMAL;
                ctx.i++;
            }
            else
            {
                accumulate_char(&ctx);
            }
        }
        else if (ctx.state == STATE_IN_DOUBLE_QUOTE)
        {
            if (ctx.buf[ctx.i] == '\"')
            {
                ctx.state = STATE_NORMAL;
                ctx.i++;
            }
            else if (ctx.buf[ctx.i] == '\\')
            {
                ctx.state = STATE_ESCAPE;
                ctx.i++;
            }
            else
            {
                accumulate_char(&ctx);
            }
        }
        else if (ctx.state == STATE_ESCAPE)
        {
            accumulate_char(&ctx);
            ctx.state = STATE_NORMAL;
        }
        else if (ctx.state == STATE_NORMAL)
        {
            if (buf[ctx.i] == '\0')
            {
                if (ctx.is_accumulate)
                {
                    word_end_process(&ctx);
                }
                break;
            }
            else if (buf[ctx.i] == ' ')
            {
                if (ctx.is_accumulate)
                {
                    word_end_process(&ctx);
                }
                ctx.i++;
            }
            else if (buf[ctx.i] == '\'')
            {
                ctx.state = STATE_IN_SINGLE_QUOTE;
                ctx.i++;
            }
            else if (ctx.buf[ctx.i] == '\"')
            {
                ctx.state = STATE_IN_DOUBLE_QUOTE;
                ctx.i++;
            }
            else if (ctx.buf[ctx.i] == '\\')
            {
                ctx.state = STATE_ESCAPE;
                ctx.i++;
            }
            else if (ctx.buf[ctx.i] == '|')
            {
                if (ctx.is_accumulate)
                {
                    word_end_process(&ctx);
                }
                if (ctx.buf[ctx.i + 1] == '|')
                {
                    or_process(&ctx);
                }
                else
                {
                    pipe_process(&ctx);
                }
            }
            else if (ctx.buf[ctx.i] == '&')
            {
                if (ctx.is_accumulate)
                {
                    word_end_process(&ctx);
                }
                if (ctx.buf[ctx.i + 1] == '&')
                {
                    and_process(&ctx);
                }
                else
                {
                    fprintf(stderr, "hsh: syntax error near unexpected token '&'\n");
                    free(ctx.tokens);
                    exit(EXIT_FAILURE);
                }
            }
            else if (ctx.buf[ctx.i] == '>')
            {
                if (ctx.is_accumulate)
                {
                    word_end_process(&ctx);
                }
                if (ctx.buf[ctx.i + 1] == '>')
                {
                    redir_append_process(&ctx);
                }
                else
                {
                    redir_out_process(&ctx);
                }
            }
            else if (ctx.buf[ctx.i] == '<')
            {
                if (ctx.is_accumulate)
                {
                    word_end_process(&ctx);
                }
                redir_in_process(&ctx);
            }
            else if (ctx.buf[ctx.i] == ';')
            {
                if (ctx.is_accumulate)
                {
                    word_end_process(&ctx);
                }
                seq_process(&ctx);
            }
            else
            {
                accumulate_char(&ctx);
            }
        }
    }
    /*
        Add the end token to the token array.
    */
    ctx.tokens[ctx.count].type = TOKEN_END;
    ctx.tokens[ctx.count].value = NULL;

    return ctx.tokens;
}

void accumulate_char(LexContext *ctx)
{
    /*
        If we are not currently accumulating characters for a token,
        start a new token.
    */
    if (!ctx->is_accumulate)
    {
        ctx->token_start = ctx->i;
        ctx->is_accumulate = true;
        ctx->i++;
    }
    else
    {
        ctx->i++;
    }
}

void ensure_token_capacity(LexContext *ctx)
{
    if (ctx->count + 1 >= ctx->capacity)
    {
        ctx->capacity *= 2;
        Token *tmp = (Token *)realloc(ctx->tokens, ctx->capacity * sizeof(Token));
        if (tmp == NULL)
        {
            free(ctx->tokens);
            fprintf(stderr, "hsh: out of memory.\n");
            exit(EXIT_FAILURE);
        }
        ctx->tokens = tmp;
        tmp = NULL;
    }
}

void word_end_process(LexContext *ctx)
{
    ensure_token_capacity(ctx);

    ctx->buf[ctx->i] = '\0';
    /*
        Add the token to the token array.
    */
    ctx->tokens[ctx->count].type = TOKEN_WORD;
    ctx->tokens[ctx->count].value = strdup(ctx->buf + ctx->token_start);
    if (ctx->tokens[ctx->count].value == NULL)
    {
        fprintf(stderr, "hsh: out of memory.\n");
        exit(EXIT_FAILURE);
    }

    ctx->count++;
    ctx->is_accumulate = false;
}

void pipe_process(LexContext *ctx)
{
    ensure_token_capacity(ctx);

    ctx->tokens[ctx->count].type = TOKEN_PIPE;
    ctx->tokens[ctx->count].value = NULL;
    ctx->count++;
    ctx->i++;
}

void redir_in_process(LexContext *ctx)
{
    ensure_token_capacity(ctx);

    ctx->tokens[ctx->count].type = TOKEN_REDIR_IN;
    ctx->tokens[ctx->count].value = NULL;
    ctx->count++;
    ctx->i++;
}

void redir_out_process(LexContext *ctx)
{
    ensure_token_capacity(ctx);

    ctx->tokens[ctx->count].type = TOKEN_REDIR_OUT;
    ctx->tokens[ctx->count].value = NULL;
    ctx->count++;
    ctx->i++;
}

void redir_append_process(LexContext *ctx)
{
    ensure_token_capacity(ctx);

    ctx->tokens[ctx->count].type = TOKEN_REDIR_APPEND;
    ctx->tokens[ctx->count].value = NULL;
    ctx->count++;
    ctx->i += 2;
}

void and_process(LexContext *ctx)
{
    ensure_token_capacity(ctx);

    ctx->tokens[ctx->count].type = TOKEN_AND_IF;
    ctx->tokens[ctx->count].value = NULL;
    ctx->count++;
    ctx->i += 2;
}

void or_process(LexContext *ctx)
{
    ensure_token_capacity(ctx);

    ctx->tokens[ctx->count].type = TOKEN_OR_IF;
    ctx->tokens[ctx->count].value = NULL;
    ctx->count++;
    ctx->i += 2;
}
void seq_process(LexContext *ctx)
{
    ensure_token_capacity(ctx);

    ctx->tokens[ctx->count].type = TOKEN_SEMICOLON;
    ctx->tokens[ctx->count].value = NULL;
    ctx->count++;
    ctx->i++;
}
