%{

#include <string.h>
#include "common.h"
#include "parser.h" 
#include <math.h>
#include <ctype.h>

void comment (); 
void codegen_copy (); 
%}


white   		[ \t]+
indent			[\n][ ]*
digit   		[0-9]
integer 		{digit}+
float   		{digit}*\.{integer}
name    		[a-zA-Z][A-Z_a-z0-9]*

%%

{indent}	 	{
  			strcpy(yylval.elem.string, yytext);
			return INDENT;
			}
{white} 		; /* ignore white space */

{integer} 		{ 
            		/* found an int */
			strcpy(yylval.elem.string, yytext);
                	yylval.elem.type = TYPE_SCALER;
			yylval.elem.dtype = TYPE_CHAN;
            		return INT;
          		} 

{float} 		{ 
          		/* found a float */
          		strcpy(yylval.elem.string, yytext);
                	yylval.elem.type = TYPE_SCALER;
                	yylval.elem.dtype = TYPE_FLOAT; 
			return FLOAT;
        		}

			
"(_c_)"			return COLOR;
"(_c_,_a_)"		return COLOR_ALPHA; 
    			
"WP"			{
			strcpy(yylval.elem.string, _WP_);
                	yylval.elem.dtype = TYPE_CHAN;
			yylval.elem.type = TYPE_SCALER;
			return(WP); 
			}
			
"ZERO_CHAN"		{
    			strcpy(yylval.elem.string, _ZERO_CHAN_);
			yylval.elem.dtype = TYPE_CHAN;
			yylval.elem.type = TYPE_SCALER;
			return(ZERO_CHAN);
			}
			
"VectorChan"		{
			strcpy(yylval.elem.string, _VectorChan_);
			yylval.elem.dtype = TYPE_CHAN;
			return(VectorChan);
			}
			
			
"Chan"			{
			strcpy(yylval.elem.string, _Chan_);
			yylval.elem.dtype = TYPE_CHAN;
			return(Chan);
			}
			
"FloatChan"		{
			strcpy(yylval.elem.string, _FloatChan_);
			yylval.elem.dtype = TYPE_FLOAT;
			return(FloatChan);  
			} 
"/*"			{ 
    			comment (); 
			} 
"/*codegen_copy"	{
    			codegen_copy ();
			}

"MAX"			return(MAX);
"MIN"			return(MIN);
"ABS"			return(ABS); 
"CHAN_CLAMP"		return(CHAN_CLAMP);
"WP_CLAMP"    		return(WP_CLAMP);
"+"			return(PLUS);
"-"			return(MINUS);
"*"			return(TIMES);
"/"			return(DIVIDE);
"^"			return(POWER);
"("			return(LT_PARENTHESIS);
")"			return(RT_PARENTHESIS);
"{"			return(LT_CURLY);
"}"			return(RT_CURLY); 
"["			return(LT_SQUARE);
"]"			return(RT_SQUARE); 
"="			return(EQUAL); 
"+="			return(PLUS_EQUAL); 
"-="			return(MINUS_EQUAL); 
"*="			return(TIMES_EQUAL); 
"/="			return(DIVIDE_EQUAL); 
"&&"			return(AND);
"||"			return(OR);
"=="			return(EQ);
"!="			return(NOT_EQ);
"<"			return(SMALLER);
">"			return(GREATER);
"<="			return(SMALLER_EQ);
">="			return(GREATER_EQ);
"!"			return(NOT);
"#"			return(POUND);
"++"			return(ADD);
"--"			return(SUBTRACT);

{name}  		{
			int keyword = get_keyword (yytext);
			switch (keyword) 
			  { 
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

"$"     		{ 
    			return 0; 
			}

.       		return yytext[0];

%%

void
comment ()
{
  int c,c1;
  
  printf ("/*");  

loop:
  while ((c = input()) != '*' && c != 0)
    {
    printf ("%c", (char)c);
    } 
  if ((c1 = input()) != '/' && c != 0)
  {
    printf ("%c", (char)c1); 
    unput(c1);
    goto loop;
  }
  
  printf ("*/"); 
}

void 
codegen_copy ()
{
 
  int c, c1[14], i=0, j;
  char word[]={'o','d','e','g','e','n','_','c','o','p','y','*','/'}; 
  char dt_word[]={'H','A','N'};  
  char cs_word[]={'U', 'E'};
  
loop:
  while (((c = input()) != 'c' && c != 'C' && c != 'H') && c != 0)
    {
    printf ("%c", (char)c);
    }
  i = 0;
  if ((char)c == 'c')
    {
    while ((c1[i] = input()) == word[i] && c != 0) 
    {
    if (i == 12)
      {
      goto done;
      } 
    i++;
    }
    printf ("%c", (char)(c)); 
    for (i=i; i>=0; i--)
      {
      unput(c1[i]);
      }
    goto loop; 

    }

  else if ((char)c == 'C')

    {
    i = 0;
    while ((c1[i] = input()) == dt_word[i] && c != 0)
      {
      if (i == 2)
        {
        printf ("%s",_CHAN_);
        goto loop; 
        }
      i++;
      }
    printf ("%c", (char)(c));
    for (i=i; i>=0; i--)
      {
      unput(c1[i]);
      }
    goto loop;
    }
  else 
    {
    i=0; 
    while ((c1[i] = input()) == cs_word[i] && c != 0)
      {
      if (i == 1)
	{
	for (j=0; j<_NUM_COLOR_CHAN_; j++)
	  {
	  printf ("%c",(char) toupper ((int)_NAME_COLOR_CHAN_[j]));
	  }
	goto loop;
	}
      i++;
      }
    printf ("%c", (char)(c));
    for (i=i; i>=0; i--)
      {
      unput(c1[i]);
      }
    goto loop;
    }
done:
}


