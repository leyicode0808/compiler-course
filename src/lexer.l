%option noyywrap yylineno

%{
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "parser.tab.h"

static int comment_start_line = 0;
static int unterminated_comment_reported = 0;

int lexical_error_count = 0;

static void print_error(const char *message, const char *lexeme) {
    printf("Error type A at Line %d: %s %s.\n",
           yylineno, message, lexeme);
    lexical_error_count++;
}

static void print_error_at_line(int line, const char *message, const char *lexeme) {
    printf("Error type A at Line %d: %s %s.\n",
           line, message, lexeme);
    lexical_error_count++;
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
FLOAT_A      {DIGIT}+"."{DIGIT}*{EXP}?
FLOAT_B      "."{DIGIT}+{EXP}?
FLOAT_C      {DIGIT}+{EXP}
FLOAT_CONST  {FLOAT_A}|{FLOAT_B}|{FLOAT_C}

%%

"//".*                  { /* skip single-line comment */ }

"/*"                    {
                            comment_start_line = yylineno;
                            unterminated_comment_reported = 0;
                            BEGIN(COMMENT);
                        }
<COMMENT>"*/"           { BEGIN(INITIAL); }
<COMMENT>\n             { /* yylineno is updated by flex */ }
<COMMENT>.              { /* skip comment body */ }
<COMMENT><<EOF>>        {
                            if (!unterminated_comment_reported) {
                                print_error_at_line(comment_start_line,
                                                    "unterminated block comment", "/*");
                                unterminated_comment_reported = 1;
                            }
                            return 0;
                        }

[ \t\r\n]+              { /* skip whitespace */ }

"int"|"float"           { yylval.text = strdup(yytext); return TYPE; }
"return"                { return RETURN; }
"if"                    { return IF; }
"else"                  { return ELSE; }
"while"                 { return WHILE; }
"struct"                { return STRUCT; }


{BAD_OCT}               { print_error("invalid octal integer", yytext); }
{FLOAT_CONST}           { yylval.text = strdup(yytext); return FLOAT; }
{HEX_INT}               { yylval.text = strdup(yytext); return INT; }
{BAD_HEX}               { print_error("invalid hexadecimal integer", yytext); }
{OCT_INT}               { yylval.text = strdup(yytext); return INT; }
{DEC_INT}               { yylval.text = strdup(yytext); return INT; }

{ID_PATTERN}            {
                            if (strlen(yytext) > 32) {
                                print_error("identifier length exceeds 32", yytext);
                            } else {
                                yylval.text = strdup(yytext);
                                return ID;
                            }
                        }

";"                     { return SEMI; }
","                     { return COMMA; }

">="|"<="|"=="|"!="|">"|"<" {
                            yylval.text = strdup(yytext);
                            return RELOP;
                        }

"&&"                    { return AND; }
"||"                    { return OR; }
"!"                     { return NOT; }

"="                     { return ASSIGNOP; }

"+"                     { return PLUS; }
"-"                     { return MINUS; }
"*"                     { return STAR; }
"/"                     { return DIV; }

"("                     { return LP; }
")"                     { return RP; }
"["                     { return LB; }
"]"                     { return RB; }
"{"                     { return LC; }
"}"                     { return RC; }
"."                     { return DOT; }

.                       { print_error("unknown character", yytext); }

%%
