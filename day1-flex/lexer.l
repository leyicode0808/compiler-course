%{
#include <stdio.h>
%}

DIGIT   [0-9]
LETTER  [A-Za-z_]
ID      {LETTER}({LETTER}|{DIGIT})*
INT     {DIGIT}+

%%

"int"       { printf("TYPE\t%s\n", yytext); }
"float"     { printf("TYPE\t%s\n", yytext); }
"return"    { printf("RETURN\t%s\n", yytext); }
"if"        { printf("IF\t%s\n", yytext); }
"else"      { printf("ELSE\t%s\n", yytext); }
"while"     { printf("WHILE\t%s\n", yytext); }

{INT}       { printf("INT\t%s\n", yytext); }
{ID}        { printf("ID\t%s\n", yytext); }

"=="|">="|"<="|"!=" { printf("RELOP\t%s\n", yytext); }
"="         { printf("ASSIGNOP\t%s\n", yytext); }
"+"|"-"|"*"|"/" { printf("OP\t%s\n", yytext); }

";"         { printf("SEMI\t%s\n", yytext); }
","         { printf("COMMA\t%s\n", yytext); }
"("         { printf("LP\t%s\n", yytext); }
")"         { printf("RP\t%s\n", yytext); }
"{"         { printf("LC\t%s\n", yytext); }
"}"         { printf("RC\t%s\n", yytext); }

[ \t\r\n]+  { /* skip whitespace */ }

.           { printf("UNKNOWN\t%s\n", yytext); }

%%

int yywrap(void) {
    return 1;
}

int main(void) {
    yylex();
    return 0;
}
