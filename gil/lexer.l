D			[0-9]
L			[a-zA-Z_]
H			[a-fA-F0-9]
E			[Ee][+-]?{D}+
FS			(f|F|l|L)
IS			(u|U|l|L)*

%{
#include <stdio.h>
#include <glib-2.0/glib.h> 
#include "parse.h"
void count();
%}

%%
"/*"			              { comment(); }
"float"		              { count(); return(FLOAT); }
"int"			              { count(); return(INT); }
{L}({L}|{D})*		        { 
                          count(); 
                          yylval.idVal = (gchar*)g_strdup(yytext); 
                          return(IDENTIFIER); 
                        }
0[xX]{H}+{IS}?	        { 
                          count(); 
                          yylval.intVal = (gint)atoi(yytext); 
                          return(INT_CONSTANT); 
                        }
0{D}+{IS}?		          { 
                          count(); 
                          yylval.intVal = (gint)atoi(yytext); 
                          return(INT_CONSTANT); 
                        }
{D}+{IS}?		            { 
                          count(); 
                          yylval.intVal = (gint)atoi(yytext); 
                          return(INT_CONSTANT); 
                        }
'(\\.|[^\\'])+'         { 
                          /* okay, what is this exactly */
                          count(); 
                          yylval.intVal = (gint)atoi(yytext); 
                          return(INT_CONSTANT); 
                        }
{D}+{E}{FS}?		        { 
                          count(); 
                          yylval.floatVal = (gfloat)atof(yytext); 
                          return(FLOAT_CONSTANT); 
                        }

{D}*"."{D}+({E})?{FS}?	{ 
                          count(); 
                          yylval.floatVal = (gfloat)atof(yytext); 
                          return(FLOAT_CONSTANT); 
                        }
{D}+"."{D}*({E})?{FS}?	{ 
                          count(); 
                          yylval.floatVal = (gfloat)atof(yytext); 
                          return(FLOAT_CONSTANT); 
                        } 
";"			                { count(); return(';'); }
"{"			                { count(); return('{'); }
"}"			                { count(); return('}'); }
","			                { count(); return(','); }
":"			                { count(); return(':'); }
"="			                { count(); return('='); }
"("			                { count(); return('('); }
")"			                { count(); return(')'); }
"["			                { count(); return('['); }
"]"			                { count(); return(']'); }
"!"			                { count(); return('!'); }
"~"			                { count(); return('~'); }
"-"			                { count(); return('-'); }
"+"			                { count(); return('+'); }
"*"			                { count(); return('*'); }
"/"			                { count(); return('/'); }
"%"			                { count(); return('%'); }
"<"			                { count(); return('<'); }
">"			                { count(); return('>'); }
"^"			                { count(); return('^'); }
"|"			                { count(); return('|'); }
"?"			                { count(); return('?'); }

[ \t\v\n\f]		{ count(); }
.			{ /* ignore bad characters */ }

%%

yywrap()
{
	return(1);
}

comment()
{
	char c, c1;

loop:
	while ((c = input()) != '*' && c != 0)
		putchar(c);

	if ((c1 = input()) != '/' && c != 0)
	{
		unput(c1);
		goto loop;
	}

	if (c != 0)
		putchar(c1);
}

int column = 0;

void count()
{
	int i;

	for (i = 0; yytext[i] != '\0'; i++)
		if (yytext[i] == '\n')
			column = 0;
		else if (yytext[i] == '\t')
			column += 8 - (column % 8);
		else
			column++;

	ECHO;
}
