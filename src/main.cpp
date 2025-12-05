#include <cstdlib>
#include <stdlib.h>

#include "ast.h"
#include "lexer.h"
#include "optutils.h"
#include "utils.h"
#include "logutils.h"

static const char* LOG_OPT = "OPTIONS";
static const char* LOG_APP = "APP";

static utils_long_opt_t long_opts[] = 
{
    { OPT_ARG_REQUIRED, "log",    NULL, 0, 0 },
    { OPT_ARG_REQUIRED, "in" ,    NULL, 0, 0 },
    { OPT_ARG_REQUIRED, "out" ,   NULL, 0, 0 },
};

int main(int argc, char* argv[])
{
    if(!utils_long_opt_get(argc, argv, long_opts, SIZEOF(long_opts)))
        return EXIT_FAILURE;

    utils_init_log_file(long_opts[0].arg, LOG_DIR);

    using namespace compiler;
    
    lexer::Lexer lex = LEXER_INITLIST;
    
    lexer::ctor(&lex);
    lexer::lex(&lex, long_opts[1].arg);

    lexer::dtor(&lex);

    utils_end_log();

    return EXIT_SUCCESS;
}
