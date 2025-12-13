#include "lexer.h"

#include <error.h>
#include <string.h>
#include <ctype.h>
#include <time.h>

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

    lex->buf.ptr = TYPED_CALLOC(fsize, char);
    lex->buf.ptr verified(return ALLOC_FAIL);

    lex->buf.filename = filename;

    size_t bytes_transferred = fread(lex->buf.ptr, sizeof(lex->buf.ptr[0]), fsize, file);
    fclose(file);
    lex->buf.len = (unsigned) bytes_transferred;
    lex->buf.ptr[lex->buf.len - 1] = '\0';

    lex_(lex);

    LEXER_DUMP(lex, ERR_NONE);

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

static size_t lex_(Lexer* lex)
{
    token::Token token = { .type = token::TYPE_FAKE, .val = token::Value { .num = 0 } };

    while(BUF_[POS_] != '\0') {

        for(size_t i = 0; i < SIZEOF(token::TokenArr); ++i) {
            if(POS_ + token::TokenArr[i].str_len < LEN_ && strncmp(token::TokenArr[i].str, BUF_ + POS_, (unsigned) token::TokenArr[i].str_len) == 0) {

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

    LEXER_DUMP(lex, ERR_NONE);

    token = { 
        .type = token::TYPE_TERMINATOR,
        .val = token::Value { .id = 0 } };

    vector_push(&lex->tokens, &token);

    return lex->tokens.size;
}

static size_t lex_numeric_(Lexer* lex)
{
    int val = 0;
    ssize_t prev = POS_;

    while(isdigit(BUF_[POS_])) {
        int digit = BUF_[POS_] - '0';
        val = val * 10 + digit;

        POS_++;
    }
    
    if(prev == POS_)
        return 0;

    token::Token tok = { 
        .type = token::TYPE_NUM_LITERAL, 
        .val  = token::Value { .num = val } };

    vector_push(&lex->tokens, &tok);

    return 1;
}

static size_t lex_identificator_(Lexer* lex)
{
    ssize_t prev = POS_;
    ssize_t len = 0;

    while(isalnum(BUF_[POS_])) {
        POS_++;
        len++;
    }

    if(prev == POS_)
        return 0;

    token::Identifier identifier = {
        .str = BUF_ + prev,
        .str_len = len,
        .hash = utils_djb2_hash(BUF_ + prev, (unsigned) len),
        .id = (signed) lex->name_table.size };

    vector_push(&lex->name_table, &identifier);

    token::Token tok = { 
        .type = token::TYPE_IDENTIFIER,
        .val = token::Value { .id = identifier.id } };

    vector_push(&lex->tokens, &tok);

    UTILS_LOGD(LOG_LEXER, "got identifier %.*s", (int)identifier.str_len, identifier.str);

    return 1;
}

#undef BUF_
#undef POS_
#undef LEN_

#ifdef _DEBUG

void dump(Lexer* lex, Err err, const char* msg, const char* filename, int line, const char* funcname)
{
    utils_log_fprintf("<pre>\n"); 

    time_t cur_time = time(NULL);
    struct tm* iso_time = localtime(&cur_time);
    char time_buff[100];
    strftime(time_buff, sizeof(time_buff), "%F %T", iso_time);

    if(err != ERR_NONE) {
        utils_log_fprintf("<h3 style=\"color:red;\">[ERROR] [%s] from %s:%d: %s() </h3>", time_buff, filename, line, funcname);
        utils_log_fprintf("<h4><font color=\"red\">err: %s </font></h4>", strerr(err));
    }
    else
        utils_log_fprintf("<h3>[DEBUG] [%s] from %s:%d: %s() </h3>\n", time_buff, filename, line, funcname);

    if(msg)
        utils_log_fprintf("what: %s\n", msg);


    BEGIN {
        if(!lex->buf.ptr) GOTO_END;

        utils_log_fprintf("buf.pos = %ld\n", lex->buf.pos); 
        utils_log_fprintf("buf.len = %ld\n", lex->buf.len); 
        utils_log_fprintf("buf.ptr[%p] = ", lex->buf.ptr); 

        if(err == INVALID_BUFPOS) {
            for(ssize_t i = 0; i < lex->buf.len; ++i)
                utils_log_fprintf("%c", lex->buf.ptr[i]);
            utils_log_fprintf("\n");
            GOTO_END;
        }

#define CLR_PREV "#09AB00"
#define CLR_CUR  "#C71022"
#define CLR_NEXT "#1022C7"

#define LOG_PRINTF_CHAR(ch, col) \
    if(ch == '\0') \
        utils_log_fprintf("<span style=\"border: 1px solid" col ";\">0</span>"); \
    else if(isspace(ch)) \
        utils_log_fprintf("<span style=\"border: 1px solid" col ";\"> </span>"); \
    else \
        utils_log_fprintf("%c", ch);



        utils_log_fprintf("<font color=\"" CLR_PREV "\">"); 
        for(ssize_t i = 0; i < lex->buf.pos; ++i) {
            LOG_PRINTF_CHAR(lex->buf.ptr[i], CLR_PREV);
        }
        utils_log_fprintf("</font>");


        utils_log_fprintf("<font color=\"" CLR_CUR "\"><b>");
        LOG_PRINTF_CHAR(lex->buf.ptr[lex->buf.pos], CLR_CUR);
        utils_log_fprintf("</b></font>");


        utils_log_fprintf("<font color=\"" CLR_NEXT "\">"); 
        for(ssize_t i = lex->buf.pos + 1; i < lex->buf.len; ++i) {
            LOG_PRINTF_CHAR(lex->buf.ptr[i], CLR_NEXT);
        }
        utils_log_fprintf("</font>\n");

    } END;

#undef CLR_PREV
#undef CLR_CUR
#undef CLR_NEXT
#undef LOG_PRINTF_CHAR

    utils_log_fprintf("\nTokens:");

    for(size_t i = 0; i < lex->tokens.size; ++i) {
        token::Token* tok = (token::Token*)vector_at(&lex->tokens, i);
        utils_log_fprintf("(%lu, %s, %s) ", i, token::type_str(tok->type), token::value_str(tok));
    }

    utils_log_fprintf("\n</pre>\n"); 

    utils_log_fprintf("<hr color=\"black\" />\n");

}

#endif // _DEBUG

} // lexer
} // compiler
