#include "lexer.h"

#include <string.h>
#include <ctype.h>

#include "assertutils.h"
#include "hashutils.h"
#include "ioutils.h"

#include "logutils.h"
#include "memutils.h"
#include "token.h"

static const char* LOG_LEXER = "LEXER";

namespace compiler {
namespace lexer {

static size_t lex_(Lexer* lex);
static size_t lex_numeric_(Lexer* lex);
static size_t lex_identificator_(Lexer* lex);

Err ctor(Lexer* lex)
{
    utils_assert(lex);

    const size_t tokens_cap = 10;
    const size_t name_table_cap = 10;
    vector_ctor(&lex->tokens, tokens_cap, sizeof(token::Token));
    vector_ctor(&lex->name_table, name_table_cap, sizeof(token::Identifier));

    return ERR_NONE;
}

Err lex(Lexer *lex, const char* filename)
{
    utils_assert(lex);
    utils_assert(filename);

    FILE* file = open_file(filename, "r");
    file verified(return IO_ERR);
    
    size_t fsize = get_file_size(file);
    UTILS_LOGI(LOG_LEXER, "%lu", fsize);

    lex->buf.ptr = TYPED_CALLOC(fsize, char);
    lex->buf.ptr verified(return ALLOC_FAIL);

    lex->buf.filename = filename;

    size_t bytes_transferred = fread(lex->buf.ptr, sizeof(lex->buf.ptr[0]), fsize, file);
    fclose(file);
    lex->buf.len = (unsigned) bytes_transferred;

    lex_(lex);

    return ERR_NONE;
}

void dtor(Lexer* lex)
{
    utils_assert(lex);

    NFREE(lex->buf.ptr);

    vector_dtor(&lex->tokens);
    vector_dtor(&lex->name_table);
}

#define BUF_ lex->buf.ptr
#define POS_ lex->buf.pos
#define LEN_ lex->buf.len

void advance_pos(Lexer* lex)
{
    if(POS_ < lex->buf.len - 1) POS_++;
}

static size_t lex_(Lexer* lex)
{
    token::Token token = { .type = token::TYPE_FAKE, .val = token::Value { .num = 0 } };

    while(BUF_[POS_] != '\n') {
        UTILS_LOGI(LOG_LEXER, "%ld", POS_);
        for(size_t i = 0; i < SIZEOF(token::TokenArr); ++i) {
            if(POS_ + token::TokenArr[i].str_len < LEN_ && strncmp(token::TokenArr[i].str, BUF_ + POS_, token::TokenArr[i].str_len) == 0) {

                token.val = token::TokenArr[i].val;
                token.type = token::TokenArr[i].type;
                vector_push(&lex->tokens, &token);

                POS_ += token::TokenArr[i].str_len;
                continue;
            }
        }

        if(lex_numeric_(lex) > 0) continue;

        if(lex_identificator_(lex) > 0) continue;

        if(isspace(BUF_[POS_])) {
            while(isspace(BUF_[POS_])) POS_++;
            continue;
        }

        // SYNTAX_ERROR
        
        UTILS_LOGE(LOG_LEXER, "syntax error");
    }

    return lex->tokens.size;
}

static size_t lex_numeric_(Lexer* lex)
{
    int val = 0;
    ssize_t prev = POS_;

    if('0' <= BUF_[POS_] && BUF_[POS_] <= '9') {
        int digit  = BUF_[POS_] - '0';
        val += val * 10 + digit;

        POS_++;
    }
    
    if(prev == POS_)
        return 0;

    token::Token tok = { 
        .type = token::TYPE_LITERAL, 
        .val = token::Value { .num = val } };

    vector_push(&lex->tokens, &tok);

    return 1;
}

static size_t lex_identificator_(Lexer* lex)
{
    ssize_t prev = POS_;
    size_t len = 0;

    while(isalnum(BUF_[POS_])) {
        POS_++;
        len++;
    }

    if(prev == POS_)
        return 0;

    token::Identifier identifier = {
        .str = BUF_ + prev,
        .str_len = len,
        .hash = utils_djb2_hash(BUF_ + prev, len),
        .id = lex->name_table.size };

    vector_push(&lex->name_table, &identifier);

    token::Token tok = { 
        .type = token::TYPE_IDENTIFIER,
        .val = token::Value { .id = identifier.id } };

    vector_push(&lex->tokens, &tok);

    UTILS_LOGD(LOG_LEXER, "got identifier %.*s", (int)identifier.str_len, identifier.str);

    return 1;
}

} // lexer
} // compiler
