#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <ctype.h>
#include "hsh.h"

static Token *lex_error(LexContext *ctx, const char *msg);
static void accumulate_char(LexContext *ctx);
static void ensure_token_capacity(LexContext *ctx);
static void word_end_process(LexContext *ctx);
static void pipe_process(LexContext *ctx);
static void redir_in_process(LexContext *ctx);
static void redir_out_process(LexContext *ctx);
static void redir_append_process(LexContext *ctx);
static void and_process(LexContext *ctx);
static void or_process(LexContext *ctx);
static void seq_process(LexContext *ctx);

static Token *lex_error(LexContext *ctx, const char *msg){
    fprintf(stderr, "hsh: %s\n", msg);
    for (int k = 0; k < ctx->count; k++)
    {
        free(ctx->tokens[k].value);
        ctx->tokens[k].value = NULL;
    }
    free(ctx->tokens);
    return NULL;
}

static void accumulate_char(LexContext *ctx)
{
    /*
        If we are not currently accumulating characters for a token,
        start a new token.
    */
    if (!ctx->is_accumulate)
    {
        ctx->token_start = ctx->j;
        ctx->is_accumulate = true;
    }
    ctx->buf[ctx->j] = ctx->buf[ctx->i];
    ctx->j++;
    ctx->i++;
}

static void ensure_token_capacity(LexContext *ctx)
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

static void word_end_process(LexContext *ctx)
{
    ensure_token_capacity(ctx);

    ctx->buf[ctx->j] = '\0';
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

static void pipe_process(LexContext *ctx)
{
    ensure_token_capacity(ctx);

    ctx->tokens[ctx->count].type = TOKEN_PIPE;
    ctx->tokens[ctx->count].value = NULL;
    ctx->count++;
    ctx->i++;
}

static void redir_in_process(LexContext *ctx)
{
    ensure_token_capacity(ctx);

    ctx->tokens[ctx->count].type = TOKEN_REDIR_IN;
    ctx->tokens[ctx->count].value = NULL;
    ctx->count++;
    ctx->i++;
}

static void redir_out_process(LexContext *ctx)
{
    ensure_token_capacity(ctx);

    ctx->tokens[ctx->count].type = TOKEN_REDIR_OUT;
    ctx->tokens[ctx->count].value = NULL;
    ctx->count++;
    ctx->i++;
}

static void redir_append_process(LexContext *ctx)
{
    ensure_token_capacity(ctx);

    ctx->tokens[ctx->count].type = TOKEN_REDIR_APPEND;
    ctx->tokens[ctx->count].value = NULL;
    ctx->count++;
    ctx->i += 2;
}

static void and_process(LexContext *ctx)
{
    ensure_token_capacity(ctx);

    ctx->tokens[ctx->count].type = TOKEN_AND_IF;
    ctx->tokens[ctx->count].value = NULL;
    ctx->count++;
    ctx->i += 2;
}

static void or_process(LexContext *ctx)
{
    ensure_token_capacity(ctx);

    ctx->tokens[ctx->count].type = TOKEN_OR_IF;
    ctx->tokens[ctx->count].value = NULL;
    ctx->count++;
    ctx->i += 2;
}
static void seq_process(LexContext *ctx)
{
    ensure_token_capacity(ctx);

    ctx->tokens[ctx->count].type = TOKEN_SEMICOLON;
    ctx->tokens[ctx->count].value = NULL;
    ctx->count++;
    ctx->i++;
}

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
    ctx.j = 0;
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
            if (ctx.buf[ctx.i] == '\0')
            {
                return lex_error(&ctx, "unterminated single quote.");
            }
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
            if (ctx.buf[ctx.i] == '\0')
            {
                return lex_error(&ctx, "unterminated double quote.");
            }
            if (ctx.buf[ctx.i] == '\"')
            {
                ctx.state = STATE_NORMAL;
                ctx.i++;
            }
            else if (ctx.buf[ctx.i] == '\\')
            {
                ctx.state = STATE_ESCAPE_IN_DOUBLE_QUOTE;
                ctx.i++;
            }
            else
            {
                accumulate_char(&ctx);
            }
        }
        else if (ctx.state == STATE_ESCAPE_IN_DOUBLE_QUOTE)
        {
            if (ctx.buf[ctx.i] == '\0')
            {
                return lex_error(&ctx, "unterminated double quote.");
            }
            accumulate_char(&ctx);
            ctx.state = STATE_IN_DOUBLE_QUOTE;
        }
        else if (ctx.state == STATE_ESCAPE)
        {
            if (ctx.buf[ctx.i] == '\0')
            {
                return lex_error(&ctx, "unterminated escape.");
            }
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
            else if (isspace((unsigned char)ctx.buf[ctx.i]))
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
                    return lex_error(&ctx, "syntax error near unexpected token '&'");
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
    ensure_token_capacity(&ctx);
    ctx.tokens[ctx.count].type = TOKEN_END;
    ctx.tokens[ctx.count].value = NULL;

    return ctx.tokens;
}
