/*
    This header file defines some processing strategies
    and practices required by the lexical analyzer.
*/

#define HSH_TOKEN_BUF_SIZE 8
#include <stdbool.h>
#include "hsh.h"

/*
    Lexical analysis states.
*/
enum LexState
{
    STATE_NORMAL,
    STATE_IN_SINGLE_QUOTE,
    STATE_IN_DOUBLE_QUOTE,
    STATE_ESCAPE
};
typedef enum LexState LexState;

/*
    Token types.
*/
enum TokenType
{
    TOKEN_WORD,
    TOKEN_PIPE,
    TOKEN_REDIR_IN,
    TOKEN_REDIR_OUT,
    TOKEN_REDIR_APPEND,
    TOKEN_AND_IF,
    TOKEN_OR_IF,
    TOKEN_SEMICOLON,
    TOKEN_END
};
typedef enum TokenType TokenType;

/*
    Token structure.
*/
struct Token
{
    TokenType type;
    char *value;
};
typedef struct Token Token;

/*
    To make the code more intuitive,
    I encapsulated some local variables of the lexical analyzer into a structure,
    which makes it easier to pass parameters to some processing functions.
*/
struct LexContext
{
    char *buf;
    Token *tokens;
    int capacity;
    int token_start;
    int count;
    int i;
    int j;
    LexState state;
    bool is_accumulate;
};
typedef struct LexContext LexContext;

/*
    Some functions used by the lexical analyzer are declared here.
*/
Token *tokenize(char *);                  // Lexical analysis function, which takes a command line string as input and returns an array of tokens.
void accumulate_char(LexContext *);       // Accumulate the current character into the current token being built.
void ensure_token_capacity(LexContext *); // Ensure that there is enough capacity in the token array, and if not, double its size.
void word_end_process(LexContext *);      // Process the end of a word token, which involves null-terminating the token string and adding it to the token array.
void pipe_process(LexContext *);          // Process a pipe token, which involves adding a TOKEN_PIPE to the token array.
void redir_in_process(LexContext *);      // Process a redirect input token, which involves adding a TOKEN_REDIR_IN to the token array.
void redir_out_process(LexContext *);     // Process a redirect output token, which involves adding a TOKEN_REDIR_OUT to the token array.
void redir_append_process(LexContext *);  // Process a redirect append token, which involves adding a TOKEN_REDIR_APPEND to the token array.
void and_process(LexContext *);           // Process an AND_IF token, which involves adding a TOKEN_AND_IF to the token array.
void or_process(LexContext *);            // Process an OR_IF token, which involves adding a TOKEN_OR_IF to the token array.
void seq_process(LexContext *);           // Process a SEQUENCE token, which involves adding a TOKEN_SEMICOLON to the token array.
