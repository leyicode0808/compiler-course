%option noyywrap yylineno

%{
#include <stdio.h>
#include <string.h>
#include "parser.tab.h"

static int comment_start_line = 0;

static void print_error(const char *message, const char *lexeme) {
    fprintf(stderr, "Lexical error at line %d: %s: %s\n",
            yylineno, message, lexeme);
}

static void print_error_at_line(int line, const char *message, const char *lexeme) {
    fprintf(stderr, "Lexical error at line %d: %s: %s\n",
            line, message, lexeme);
}
%}

%x COMMENT

DIGIT        [0-9]
LETTER       [A-Za-z_]
ID_PATTERN   {LETTER}({LETTER}|{DIGIT})*

HEX_DIGIT    [0-9A-Fa-f]
DEC_INT      0|[1-9]{DIGIT}*
OCT_INT      0[0-7]+
HEX_INT      0[xX]{HEX_DIGIT}+
BAD_HEX      0[xX][0-9A-Za-z_]*
BAD_OCT      0[0-7]*[89]{DIGIT}*

EXP          [eE][+-]?{DIGIT}+
FLOAT_SIMPLE {DIGIT}+"."{DIGIT}+
FLOAT_EXP_A  {DIGIT}+"."{DIGIT}*{EXP}
FLOAT_EXP_B  {DIGIT}*"."{DIGIT}+{EXP}
FLOAT_CONST  {FLOAT_SIMPLE}|{FLOAT_EXP_A}|{FLOAT_EXP_B}

%%

"//".*                  { /* skip single-line comment */ }

"/*"                    {
                            comment_start_line = yylineno;
                            BEGIN(COMMENT);
                        }
<COMMENT>"*/"           { BEGIN(INITIAL); }
<COMMENT>\n             { /* yylineno is updated by flex */ }
<COMMENT>.              { /* skip comment body */ }
<COMMENT><<EOF>>        {
                            print_error_at_line(comment_start_line,
                                                "unterminated block comment", "/*");
                            return 0;
                        }

[ \t\r\n]+              { /* skip whitespace */ }

"int"|"float"           { return TYPE; }
"return"                { return RETURN; }
"if"                    { return IF; }
"else"                  { return ELSE; }
"while"                 { return WHILE; }

{BAD_OCT}               { print_error("invalid octal integer", yytext); }
{FLOAT_CONST}           { print_error("float literal is not supported in minimal parser", yytext); }
{HEX_INT}               { return INT; }
{BAD_HEX}               { print_error("invalid hexadecimal integer", yytext); }
{OCT_INT}               { return INT; }
{DEC_INT}               { return INT; }

{ID_PATTERN}            {
                            if (strlen(yytext) > 32) {
                                print_error("identifier length exceeds 32", yytext);
                            } else {
                                return ID;
                            }
                        }

";"                     { return SEMI; }

">="|"<="|"=="|"!="|">"|"<" {
                            return RELOP;
                        }

"="                     { return ASSIGNOP; }

"+"                     { return PLUS; }
"-"                     { return MINUS; }
"*"                     { return STAR; }
"/"                     { return DIV; }

"("                     { return LP; }
")"                     { return RP; }
"{"                     { return LC; }
"}"                     { return RC; }

.                       { print_error("unknown character", yytext); }

%%
