%{
#ifndef _LEXER_C_
#define _LEXER_C_
#endif

#include <string.h>
#include "common.h"
#include "parser.h" 
#include <math.h>
#include <ctype.h>
#include <stdio.h>

int PREV;  
%}

%s			GENERIC_IMAGE_DECL
%s			GENERIC_IMAGE_CODE
%s			COMMENT 
%s			DT_MACROS 
%s			ARGUMENTS
%s			DEFINITION
%s 			EXTERNAL_VARIABLE 
white   		[ \t]+
indent			[\n][ \t]*
cont			("\\""\n")    
digit   		[0-9]
integer 		{digit}+
float   		{digit}*\.{integer}
name    		[a-zA-Z][A-Z_a-z0-9]*
Name			[A-Z][_A-Z0-9]*
string			(.*{cont}*)* 	
space			[]*[\t]+
rm			[\t \n]* 
%%

{rm}"DT_MACROS_BEGIN"			BEGIN DT_MACROS;  
{rm}"GENERIC_IMAGE_DECL_BEGIN"		{
  					/* wacky stuff */
  					int i,j;
  					i = strlen (yytext);
					while (i>=0 && yytext[i] != '\n')
					  i--;
					for (j=0; j<i; j++)
					 printf ("%c", yytext[j]);  
					      
					BEGIN GENERIC_IMAGE_DECL;
					}
{indent}"GENERIC_IMAGE_CODE_BEGIN"	BEGIN GENERIC_IMAGE_CODE;
{indent}"GENERIC_IMAGE_IMAGE_DATA_INIT" {
  					char tmp[255];
					strcpy (tmp, yytext);
					tmp[strlen (tmp) - strlen ("GENERIC_IMAGE_IMAGE_DATA_INIT")] = '\0'; 
  					init_image_data (tmp);
					}	
<DT_MACROS>{indent}			;         	
<DT_MACROS>{rm}"DT_MACROS_END"{rm}	BEGIN 0; 

<ARGUMENTS>","				return DT_COMMA; 
<ARGUMENTS>"("				return LT_PARENTHESIS;
<ARGUMENTS>")"				{BEGIN DEFINITION; return RT_PARENTHESIS;}  


<GENERIC_IMAGE_CODE>{indent}"{"	{
    					SCOPE++; 
    					strcpy(yylval.elem.string, yytext);
					return INDENT_CURLY;
					}
<GENERIC_IMAGE_DECL>{indent}          	{
                            		strcpy(yylval.elem.string, yytext);
			       	 	return INDENT;
			        	}
<GENERIC_IMAGE_CODE>{indent}         	{
                                	strcpy(yylval.elem.string, yytext);
					return INDENT;
					}

			    	 	
<GENERIC_IMAGE_DECL>{white} 		; 
<GENERIC_IMAGE_CODE>{white} 		; 

<GENERIC_IMAGE_DECL>{rm}"GENERIC_IMAGE_DECL_END" 	BEGIN 0;
<GENERIC_IMAGE_CODE>{rm}"GENERIC_IMAGE_CODE_END" 	BEGIN 0;

<GENERIC_IMAGE_CODE>"WP"		{
					strcpy(yylval.elem.string, WP_STR);
                			yylval.elem.dtype = TYPE_CHANNEL;
					yylval.elem.svtype = TYPE_VECTOR;
					return(WP); 
					}
<GENERIC_IMAGE_CODE>"ZERO"		{
    					strcpy(yylval.elem.string, ZERO_STR);
					yylval.elem.dtype = TYPE_CHANNEL;
			        	yylval.elem.svtype = TYPE_VECTOR;
					return ZERO; 	
					} 
<GENERIC_IMAGE_DECL>"Pixel"		{
					strcpy(yylval.elem.string, DATATYPE_STR);
					yylval.elem.dtype = TYPE_CHANNEL;
					return(Pixel);
					}
<GENERIC_IMAGE_DECL>"Channel"		{
    					strcpy(yylval.elem.string, DATATYPE_STR);
					yylval.elem.dtype = TYPE_CHANNEL;
					return(Channel);
					}

<GENERIC_IMAGE_DECL>{integer}		{ 
            				/* found an int */
					strcpy(yylval.elem.string, yytext);
                			yylval.elem.svtype = TYPE_SCALER;
					yylval.elem.dtype = TYPE_INT;
            				return INT;
					}
<GENERIC_IMAGE_DECL>{float} 		{ 
          				/* found a float */
          				strcpy(yylval.elem.string, yytext);
                			yylval.elem.svtype = TYPE_SCALER;
                			yylval.elem.dtype = TYPE_FLOAT; 
					return FLOAT;
        				}
<GENERIC_IMAGE_CODE>{integer}		{ 
            				/* found an int */
					strcpy(yylval.elem.string, yytext);
                			yylval.elem.svtype = TYPE_SCALER;
					yylval.elem.dtype = TYPE_INT;
            				return INT;
          				} 
<GENERIC_IMAGE_CODE>{float} 		{ 
          				/* found a float */
          				strcpy(yylval.elem.string, yytext);
                			yylval.elem.svtype = TYPE_SCALER;
                			yylval.elem.dtype = TYPE_FLOAT; 
					return FLOAT;
        				}

<GENERIC_IMAGE_DECL>"/*"		{ECHO; PREV=0; BEGIN COMMENT;}
<GENERIC_IMAGE_CODE>"/*"		{ECHO; PREV=1; BEGIN COMMENT;}
<COMMENT>"*/"				{
    					ECHO; 
					if (PREV) BEGIN GENERIC_IMAGE_CODE; 
    					else BEGIN GENERIC_IMAGE_DECL;
					} 

<GENERIC_IMAGE_DECL>"(color,alpha,has_alpha)"	return(COLOR_MAYBE_ALPHA);
<GENERIC_IMAGE_DECL>"(color,alpha)"		return(COLOR_ALPHA);
<GENERIC_IMAGE_DECL>"(color)"			return(COLOR);

<GENERIC_IMAGE_CODE>"dX"		return(ITERATOR_X);
<GENERIC_IMAGE_CODE>"dY"		return(ITERATOR_Y);
<GENERIC_IMAGE_CODE>"dXY"            	return(ITERATOR_XY);

<GENERIC_IMAGE_CODE>"MAX"		{	
    					strcpy (yylval.tok.string, yytext); 
    					return (MIN_MAX);
					}
<GENERIC_IMAGE_CODE>"MIN"		{	
    					strcpy (yylval.tok.string, yytext); 
    					return (MIN_MAX);
					}
<GENERIC_IMAGE_CODE>"EXTERNAL_INIT"	BEGIN EXTERNAL_VARIABLE; 	

<GENERIC_IMAGE_CODE>"(Promote)"	return(PROMOTE_TYPE); 
<GENERIC_IMAGE_CODE>"(SignedPromote)"	return(SIGNED_PROMOTE_TYPE); 
<GENERIC_IMAGE_CODE>"ABS"		return(ABS); 
<GENERIC_IMAGE_CODE>"PRINT"		return(PRINT); 
<GENERIC_IMAGE_CODE>"CHANNEL_CLAMP"	return(CHANNEL_CLAMP);
<GENERIC_IMAGE_CODE>"WP_CLAMP"    	return(WP_CLAMP);


<GENERIC_IMAGE_CODE>"+"		return(PLUS);
<GENERIC_IMAGE_CODE>"-"		return(MINUS);
<GENERIC_IMAGE_DECL>"*"			return(TIMES);
<GENERIC_IMAGE_CODE>"*"		return(TIMES);
<GENERIC_IMAGE_CODE>"/"		return(DIVIDE);
<GENERIC_IMAGE_CODE>"^"		return(POWER);
<GENERIC_IMAGE_CODE>"("		return(LT_PARENTHESIS);
<GENERIC_IMAGE_CODE>")"		return(RT_PARENTHESIS);
<EXTERNAL_VARIABLE>"("			;
<EXTERNAL_VARIABLE>")"			BEGIN GENERIC_IMAGE_CODE;
<GENERIC_IMAGE_CODE>"{"		{SCOPE ++; return(LT_CURLY);}
<GENERIC_IMAGE_CODE>"}"		{SCOPE --; rm_varibles (SCOPE); return(RT_CURLY); } 
<GENERIC_IMAGE_DECL>"{"			{SCOPE ++; ECHO;}
<GENERIC_IMAGE_DECL>"}"			{SCOPE --; rm_varibles (SCOPE); ECHO;} 
"{"					{SCOPE ++; ECHO;}
"}"					{SCOPE --; rm_varibles (SCOPE); ECHO;} 
<GENERIC_IMAGE_DECL>"["			return(LT_SQUARE);
<GENERIC_IMAGE_CODE>"["		return(LT_SQUARE);
<GENERIC_IMAGE_DECL>"]"			return(RT_SQUARE); 
<GENERIC_IMAGE_CODE>"]"		return(RT_SQUARE); 
<GENERIC_IMAGE_DECL>"="			return(EQUAL); 
<GENERIC_IMAGE_CODE>"="		return(EQUAL); 
<GENERIC_IMAGE_CODE>"+="		return(PLUS_EQUAL); 
<GENERIC_IMAGE_CODE>"-="		return(MINUS_EQUAL); 
<GENERIC_IMAGE_CODE>"*="		return(TIMES_EQUAL); 
<GENERIC_IMAGE_CODE>"/="		return(DIVIDE_EQUAL); 
<GENERIC_IMAGE_CODE>"&&"		{
    					strcpy (yylval.tok.string, yytext); 
    					return (COMPARE);
					}
<GENERIC_IMAGE_CODE>"||"		{
    					strcpy (yylval.tok.string, yytext); 
    					return (COMPARE);
					}
<GENERIC_IMAGE_CODE>"=="		{
    					strcpy (yylval.tok.string, yytext); 
    					return (COMPARE);
					}
<GENERIC_IMAGE_CODE>"!="		{
    					strcpy (yylval.tok.string, yytext); 
    					return (COMPARE);
					}
<GENERIC_IMAGE_CODE>"<"		{
    					strcpy (yylval.tok.string, yytext); 
    					return (COMPARE);
					}
<GENERIC_IMAGE_CODE>">"		{
    					strcpy (yylval.tok.string, yytext); 
    					return (COMPARE);
					}
<GENERIC_IMAGE_CODE>"<="		{
    					strcpy (yylval.tok.string, yytext); 
    					return (COMPARE);
					}
<GENERIC_IMAGE_CODE>">="		{	
    					strcpy (yylval.tok.string, yytext); 
    					return (COMPARE);
					}
<GENERIC_IMAGE_CODE>"!"		return(NOT);
<GENERIC_IMAGE_CODE>"++"		{
    					strcpy (yylval.tok.string, yytext); 
    					return (ADD_SUB);
					}
<GENERIC_IMAGE_CODE>"--"		{
    					strcpy (yylval.tok.string, yytext); 
    					return (ADD_SUB);
					}

<GENERIC_IMAGE_DECL>{name}  		{
					int keyword = get_keyword (yytext);
					switch (keyword) 
			  		{ 
			  		case -1:
			    		/* return symbol pointer */
			    		yylval.elem = add_sym (yytext, SCOPE);
			    		strcpy(yylval.elem.string, yytext);
					return NAME;
			    		break;
			  		default:
			    		return get_keyword (yytext);
			    		break; 		  
			  		}
          				}
<GENERIC_IMAGE_CODE>{Name}  		{
					int keyword = get_keyword (yytext);
					switch (keyword) 
			  		{ 
			  		case -1:
			    		/* return symbol pointer */
			    		strcpy(yylval.elem.string, yytext); 
			    		return NAME;
			    		break;
			  		default:
			    		return get_keyword (yytext);
			    		break; 		  
			  		}
          				}

<GENERIC_IMAGE_CODE>{name}  		{
					int keyword = get_keyword (yytext);
					switch (keyword) 
			  		{ 
			  		case -1:
			    		/* return symbol pointer */
			    		yylval.elem = *(get_sym (yytext));
			    		strcpy(yylval.elem.string, yytext); 
			    		return NAME;
			    		break;
			  		default:
			    		return get_keyword (yytext);
			    		break; 		  
			  		}
          				}
<ARGUMENTS>{name}			{
    					strcpy (yylval.tok.string, yytext);
					return DT_NAME;
					}
<DT_MACROS>{Name}			{
      					dt_keyword_t keywd;
      					keywd = get_dt_keyword (yytext);
				        if (keywd.token == -1) 	
					  return 0;
					if (keywd.arg)
					  BEGIN ARGUMENTS;
					else
					  BEGIN DEFINITION;
					return keywd.token;
					}
<EXTERNAL_VARIABLE>{name}		{
    					yylval.elem.num = 0;
					yylval.elem.dtype = TYPE_CHANNEL;
					yylval.elem.svtype = TYPE_VECTOR;
					sprintf (yylval.elem.string, "%s$e.%s", 
					      yytext, EXTERNAL_INIT_STR);
				       	return VARIABLE; 	
      					}
<DEFINITION>{string}			{
    					strcpy (yylval.tok.string, yytext);
					BEGIN DT_MACROS; 
					return DT_STRING;
					}


{rm}"$" 				{BEGIN 0; return 0;} 
<GENERIC_IMAGE_DECL>.			return yytext[0];
<GENERIC_IMAGE_CODE>.			return yytext[0];
<COMMENT>.				ECHO;
<DT_MACROS>.				;
<ARGUMENTS>.				;

.					ECHO; 	


%%

void
open_file (char *filename)
{
  if (!strcmp (filename, "stdin"))
    yyin = stdin; 
  else
    yyin = fopen (filename, "r"); 
}

void
close_file ()
{
 fclose (yyin); 
}


