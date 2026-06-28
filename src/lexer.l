%option noyywrap yylineno

%{
#include <stdio.h>
#include <string.h>

static void print_token(const char *token, const char *lexeme) {
    printf("Line %-3d %-10s %s\n", yylineno, token, lexeme);
}

static void print_error(const char *message, const char *lexeme) {
    fprintf(stderr, "Lexical error at line %d: %s: %s\n",
            yylineno, message, lexeme);
}
%}

%x COMMENT

DIGIT       [0-9]
LETTER      [A-Za-z_]
ID          {LETTER}({LETTER}|{DIGIT})*

HEX_DIGIT   [0-9A-Fa-f]
DEC_INT     0|[1-9]{DIGIT}*
OCT_INT     0[0-7]+
HEX_INT     0[xX]{HEX_DIGIT}+
BAD_OCT     0[0-7]*[89]{DIGIT}*

EXP          [eE][+-]?{DIGIT}+
FLOAT_SIMPLE {DIGIT}+"."{DIGIT}+
FLOAT_EXP_A  {DIGIT}+"."{DIGIT}*{EXP}
FLOAT_EXP_B  {DIGIT}*"."{DIGIT}+{EXP}
FLOAT_CONST  {FLOAT_SIMPLE}|{FLOAT_EXP_A}|{FLOAT_EXP_B}

%%

"//".*                  { /* skip single-line comment */ }

"/*"                    { BEGIN(COMMENT); }
<COMMENT>"*/"           { BEGIN(INITIAL); }
<COMMENT>\n             { /* yylineno is updated by flex */ }
<COMMENT>.              { /* skip comment body */ }
<COMMENT><<EOF>>        {
                            print_error("unterminated block comment", "/*");
                            return 0;
                        }

[ \t\r\n]+              { /* skip whitespace */ }

"int"|"float"           { print_token("TYPE", yytext); }
"struct"                { print_token("STRUCT", yytext); }
"return"                { print_token("RETURN", yytext); }
"if"                    { print_token("IF", yytext); }
"else"                  { print_token("ELSE", yytext); }
"while"                 { print_token("WHILE", yytext); }

{BAD_OCT}               { print_error("invalid octal integer", yytext); }
{FLOAT_CONST}           { print_token("FLOAT", yytext); }
{HEX_INT}               { print_token("INT", yytext); }
{OCT_INT}               { print_token("INT", yytext); }
{DEC_INT}               { print_token("INT", yytext); }

{ID}                    {
                            if (strlen(yytext) > 32) {
                                print_error("identifier length exceeds 32", yytext);
                            } else {
                                print_token("ID", yytext);
                            }
                        }

";"                     { print_token("SEMI", yytext); }
","                     { print_token("COMMA", yytext); }
"="                     { print_token("ASSIGNOP", yytext); }

">="|"<="|"=="|"!="|">"|"<" {
                            print_token("RELOP", yytext);
                        }

"+"                     { print_token("PLUS", yytext); }
"-"                     { print_token("MINUS", yytext); }
"*"                     { print_token("STAR", yytext); }
"/"                     { print_token("DIV", yytext); }
"&&"                    { print_token("AND", yytext); }
"||"                    { print_token("OR", yytext); }
"."                     { print_token("DOT", yytext); }
"!"                     { print_token("NOT", yytext); }

"("                     { print_token("LP", yytext); }
")"                     { print_token("RP", yytext); }
"["                     { print_token("LB", yytext); }
"]"                     { print_token("RB", yytext); }
"{"                     { print_token("LC", yytext); }
"}"                     { print_token("RC", yytext); }

.                       { print_error("unknown character", yytext); }

%%

int main(int argc, char **argv) {
    if (argc > 1) {
        yyin = fopen(argv[1], "r");
        if (yyin == NULL) {
            perror(argv[1]);
            return 1;
        }
    }

    yylex();

    if (argc > 1) {
        fclose(yyin);
    }

    return 0;
}
