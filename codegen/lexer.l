%{

#include <string.h>
#include "common.h"
#include "parser.h" 
#include <math.h>

void comment (); 
%}


white   	[ \t]+
digit   	[0-9]
integer 	{digit}+
float   	{digit}*\.{integer}?
name    	[a-zA-Z][A-Z_a-z0-9]*

%%

{white} 	; /* ignore white space */

{integer} 	{ 
            	/* found an int */
		strcpy(yylval.elem.string, yytext);
                yylval.elem.type = TYPE_SCALER;
		yylval.elem.dtype = TYPE_INT;
            	return INT;
          	} 

{float} 	{ 
          	/* found a float */
          	strcpy(yylval.elem.string, yytext);
                yylval.elem.type = TYPE_SCALER;
                yylval.elem.dtype = TYPE_FLOAT; 
		return FLOAT;
        	}


"WP"		{
		strcpy(yylval.elem.string, _WP_);
                yylval.elem.dtype = TYPE_INT;
		yylval.elem.type = TYPE_SCALER;
		return(WP); 
		}		
"ZERO_CHAN"	{
    		strcpy(yylval.elem.string, _ZERO_CHAN_);
		yylval.elem.dtype = TYPE_INT;
		yylval.elem.type = TYPE_SCALER;
		return(ZERO_CHAN);
		}
"Chan"		{
		strcpy(yylval.elem.string, _Chan_);
		yylval.elem.dtype = TYPE_INT;
		return(Chan);
		}
"FloatChan"	{
		strcpy(yylval.elem.string, _FloatChan_);
		yylval.elem.dtype = TYPE_FLOAT;
		return(FloatChan);  
		} 

"MAX"		return(MAX);
"MIN"		return(MIN);
"ABS"		return(ABS); 
"CHAN_CLAMP"	return(CHAN_CLAMP);
"WP_CLAMP"    	return(WP_CLAMP);
"+"		return(PLUS);
"-"		return(MINUS);
"*"		return(TIMES);
"/"		return(DIVIDE);
"^"		return(POWER);
"("		return(LT_PARENTHESIS);
")"		return(RT_PARENTHESIS);
"{"		return(LT_CURLY);
"}"		return(RT_CURLY); 
"="		return(EQUAL); 
"+="		return(PLUS_EQUAL); 
"-="		return(MINUS_EQUAL); 
"*="		return(TIMES_EQUAL); 
"/="		return(DIVIDE_EQUAL); 
"&&"		return(AND);
"||"		return(OR);
"=="		return(EQ);
"!="		return(NOT_EQ);
"<"		return(SMALLER);
">"		return(GREATER);
"<="		return(SMALLER_EQ);
">="		return(GREATER_EQ);
"!"		return(NOT);


{name}  	{
		int keyword = get_keyword (yytext);
		switch (keyword) { 
		  case -1:
		    /* return symbol pointer */
		    yylval.elem = add_sym (yytext);
		    strcpy(yylval.elem.string, yytext); 
          	    return NAME;
                  break;
		  default:
		    return get_keyword (yytext);
		  break; 		  
		}
        	}

"$"     	{ return 0; }

"\n"		return(END);

.       	return yytext[0];

%%

void
comment()
{
  int c,c1;
  
  printf ("/*");  
  loop:
  while ((c = input()) != '*' && c != 0)
    {
    printf ("%c", (char)c);
    } 
  /*
     putchar(c);
  */
  if ((c1 = input()) != '/' && c != 0)
  {
    printf ("%c", (char)c1); 
    unput(c1);
    goto loop;
  }
  
  printf ("*/"); 
  /*
    if (c != 0)
      putchar(c1);
  */
}
