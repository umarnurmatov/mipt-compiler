#include <cstdlib>
#include <error.h>
#include <stdlib.h>

#include "ast.h"
#include "ioutils.h"
#include "optutils.h"
#include "utils.h"
#include "logutils.h"
#include "translator.h"

ATTR_UNUSED static const char* LOG_OPT = "OPTIONS";
ATTR_UNUSED static const char* LOG_APP = "APP";

static utils_long_opt_t long_opts[] = 
{
    { OPT_ARG_REQUIRED, "log",    NULL, 0, 0 },
    { OPT_ARG_REQUIRED, "in" ,    NULL, 0, 0 },
    { OPT_ARG_REQUIRED, "out" ,   NULL, 0, 0 },
};

#ifdef _DEBUG

static void log_html_style();

#endif // _DEBUG

int main(int argc, char* argv[])
{
    if(!utils_long_opt_get(argc, argv, long_opts, SIZEOF(long_opts)))
        return EXIT_FAILURE;

    utils_init_log_file(long_opts[0].arg, LOG_DIR);

    IF_DEBUG(log_html_style());

    using namespace compiler;

    Err err = ERR_NONE;

    ast::AST astree = AST_INITLIST;
    ast::ctor(&astree);

    Translator tr = TRANSLATOR_INILIST;
    tr.astree = &astree;

    bool err_occured = false;

    BEGIN {

        FILE* file_ast = open_file(long_opts[1].arg, "r");
        if(!file_ast) {
            err_occured = true;
            GOTO_END;
        }

        err = ast::fread_infix(&astree, file_ast, long_opts[1].arg);

        fclose(file_ast);

        if(err != ERR_NONE) {
            err_occured = true;
            GOTO_END;
        }

        FILE* file_asm = open_file(long_opts[2].arg, "w");
        if(!file_ast) {
            err_occured = true;
            GOTO_END;
        }
        tr.file = file_asm;

        emit_program(&tr);

        fclose(file_asm);

    } END;

    ast::dtor(&astree);

    utils_end_log();

    return err_occured ? EXIT_FAILURE : EXIT_SUCCESS;
}

#ifdef _DEBUG

static void log_html_style()
{
    utils_log_fprintf(
        "<style>\n"
        "table {\n"
          "border-collapse: collapse;\n"
          "border: 1px solid;\n"
          "font-size: 0.9em;\n"
        "}\n"
        "th,\n"
        "td {\n"
          "border: 1px solid rgb(160 160 160);\n"
          "padding: 8px 10px;\n"
        "}\n"
        "</style>\n"
    );
}

#endif // _DEBUG
