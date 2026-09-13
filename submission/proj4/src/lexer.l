%option noyywrap yylineno nodefault noinput nounput

%{
#include <stdio.h>
#include <string.h>

#ifdef PARSER_BUILD
#include "ast.h"
#include "parser.tab.h"
#else
#include "token.h"
#endif

int lexical_error_count = 0;
static int block_comment_start_line = 1;

static void lexical_error(const char *message) {
    printf("Error type A at Line %d: %s.\n", yylineno, message);
    lexical_error_count++;
}

static void lexical_error_text(const char *message, const char *text) {
    printf("Error type A at Line %d: %s '%s'.\n", yylineno, message, text);
    lexical_error_count++;
}

#ifdef PARSER_BUILD
#define TOKEN(name) do { yylval.node = ast_token(#name, NULL, yylineno); return name; } while (0)
#define TOKEN_VALUE(name) do { yylval.node = ast_token(#name, yytext, yylineno); return name; } while (0)
#else
#define TOKEN(name) return TOK_##name
#define TOKEN_VALUE(name) return TOK_##name
#endif
%}

DIGIT       [0-9]
NZDIGIT     [1-9]
LETTER      [_a-zA-Z]
IDCHAR      [_a-zA-Z0-9]
DEC_INT     0|{NZDIGIT}{DIGIT}*
OCT_INT     0[0-7]+
HEX_INT     0[xX][0-9a-fA-F]+
EXP         [eE][+-]?{DIGIT}+
FLOAT_BASE  ({DIGIT}+\.{DIGIT}+|{DIGIT}+\.|\.{DIGIT}+)
FLOAT_LIT   ({FLOAT_BASE}{EXP}?|{DIGIT}+{EXP})
ID          {LETTER}{IDCHAR}*

%x COMMENT

%%

[ \t\r\f\v]+              ;
\n                        ;

"//"[^\n]*                ;
"/*"                      { block_comment_start_line = yylineno; BEGIN(COMMENT); }
<COMMENT>"/*"             { lexical_error("Nested block comment is not allowed"); }
<COMMENT>"*/"             { BEGIN(INITIAL); }
<COMMENT>\n               ;
<COMMENT>.                ;
<COMMENT><<EOF>>          {
                              printf("Error type A at Line %d: Unterminated block comment.\n",
                                     block_comment_start_line);
                              lexical_error_count++;
                              BEGIN(INITIAL);
                              return 0;
                          }

"struct"                  { TOKEN(STRUCT); }
"return"                  { TOKEN(RETURN); }
"if"                      { TOKEN(IF); }
"else"                    { TOKEN(ELSE); }
"while"                   { TOKEN(WHILE); }
"int"|"float"             { TOKEN_VALUE(TYPE); }

"&&"                      { TOKEN(AND); }
"||"                      { TOKEN(OR); }
">="|"<="|"=="|"!="|">"|"<" { TOKEN_VALUE(RELOP); }
"="                       { TOKEN(ASSIGNOP); }
";"                       { TOKEN(SEMI); }
","                       { TOKEN(COMMA); }
"+"                       { TOKEN(PLUS); }
"-"                       { TOKEN(MINUS); }
"*"                       { TOKEN(STAR); }
"/"                       { TOKEN(DIV); }
"."                       { TOKEN(DOT); }
"!"                       { TOKEN(NOT); }
"("                       { TOKEN(LP); }
")"                       { TOKEN(RP); }
"["                       { TOKEN(LB); }
"]"                       { TOKEN(RB); }
"{"                       { TOKEN(LC); }
"}"                       { TOKEN(RC); }

0[xX][0-9a-fA-F]*[g-zG-Z_]{IDCHAR}* { lexical_error_text("Invalid hexadecimal literal", yytext); }
0[xX]                    { lexical_error_text("Invalid hexadecimal literal", yytext); }
0[0-7]*[89]{IDCHAR}*     { lexical_error_text("Invalid octal literal", yytext); }
{DIGIT}+\.{DIGIT}+\.{DIGIT}({DIGIT}|\.)* { lexical_error_text("Invalid floating point literal", yytext); }
({FLOAT_BASE}|{DIGIT}+)[eE][+-]?{LETTER}{IDCHAR}* { lexical_error_text("Invalid floating point literal", yytext); }
({FLOAT_BASE}|{DIGIT}+)[eE][+-]? { lexical_error_text("Invalid floating point literal", yytext); }

{FLOAT_LIT}               { TOKEN_VALUE(FLOAT); }
{HEX_INT}|{OCT_INT}|{DEC_INT} { TOKEN_VALUE(INT); }
{DIGIT}+{LETTER}{IDCHAR}* { lexical_error_text("Invalid identifier starting with digit", yytext); }
{ID}                      { TOKEN_VALUE(ID); }

.                         { lexical_error_text("Undefined character", yytext); }

%%
