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

void print_info (char *line); 
int PREV;  
%}

%s			GENERIC_IMAGE_DECL
%s			GENERIC_IMAGE_BLOCK
%s			COMMENT 
%s			DATA_TYPE 
white   		[ \t]+
indent			[\n][ \t]*
digit   		[0-9]
integer 		{digit}+
float   		{digit}*\.{integer}
name    		[a-zA-Z][A-Z_a-z0-9]*
Name			[A-Z][_A-Z0-9]*
string			[^\t].*	
space			[]*[\t]+
rm			[\t \n]* 
%%

{rm}"DATATYPE_BEGIN"			BEGIN DATA_TYPE;  
{indent}"GENERIC_IMAGE_DECL_BEGIN"	{
  					print_info ("\n/* Generic Image Decl Begin */");
					BEGIN GENERIC_IMAGE_DECL;
					}
{indent}"GENERIC_IMAGE_BLOCK_BEGIN"	{
					print_info ("\n/* Generic Image Block Begin */"); 
					BEGIN GENERIC_IMAGE_BLOCK;
					}
{indent}"GENERIC_IMAGE_IMAGE_DATA_INIT" {
  					char tmp[255];
  					print_info ("\n/* Generic Image Image Data Init */"); 
					strcpy (tmp, yytext);
					tmp[strlen (tmp) - strlen ("GENERIC_IMAGE_IMAGE_DATA_INIT")] = '\0'; 
  					init_image_data (tmp);
					}	
<DATA_TYPE>{indent}			;         	
<DATA_TYPE>"DATATYPE_END""\n"		{BEGIN 0;} 
<DATA_TYPE>"DATATYPE"			return DT_DATATYPE;
<DATA_TYPE>"WP"				return DT_WP;
<DATA_TYPE>"WP_NORM"			return DT_WP_NORM;
<DATA_TYPE>"MIN_CHANNEL"		return DT_MIN_CHANNEL;
<DATA_TYPE>"MAX_CHANNEL"                return DT_MAX_CHANNEL;
<DATA_TYPE>"ZERO"			return DT_ZERO; 
<DATA_TYPE>"CHANNEL_CLAMP"		return DT_CHANNEL_CLAMP;
<DATA_TYPE>"WP_CLAMP"			return DT_WP_CLAMP;
<DATA_TYPE>"CHANNEL_MULT"		return DT_CHANNEL_MULT;
<DATA_TYPE>"ROUND_CHANNEL"		return DT_ROUND_CHANNEL;
<DATA_TYPE>","				return DT_COMMA; 
<DATA_TYPE>"("				return LT_PARENTHESIS;
<DATA_TYPE>")"				return RT_PARENTHESIS; 


<GENERIC_IMAGE_BLOCK>{indent}"{"	{
    					SCOPE++; 
    					strcpy(yylval.elem.string, yytext);
					return INDENT_CURLY;
					}
<GENERIC_IMAGE_DECL>{indent}          	{
                            		strcpy(yylval.elem.string, yytext);
			       	 	return INDENT;
			        	}
<GENERIC_IMAGE_BLOCK>{indent}         	{
                                	strcpy(yylval.elem.string, yytext);
					return INDENT;
					}

			    	 	
<GENERIC_IMAGE_DECL>{white} 		; 
<GENERIC_IMAGE_BLOCK>{white} 		; 

<GENERIC_IMAGE_DECL>{indent}"GENERIC_IMAGE_DECL_END" 	{
    							print_info ("\n/*Generic Image Decl End */\n");
							BEGIN 0;
							}
<GENERIC_IMAGE_BLOCK>{indent}"GENERIC_IMAGE_BLOCK_END" 	{
    							print_info ("\n/*Generic Image Block End */\n");
							BEGIN 0;
							}

<GENERIC_IMAGE_BLOCK>"WP"		{
					strcpy(yylval.elem.string, WP_STR);
                			yylval.elem.dtype = TYPE_CHANNEL;
					yylval.elem.type = TYPE_VECTOR;
					return(WP); 
					}
<GENERIC_IMAGE_BLOCK>"ZERO"		{
    					strcpy(yylval.elem.string, ZERO_STR);
					yylval.elem.dtype = TYPE_CHANNEL;
			        	yylval.elem.type = TYPE_VECTOR;
					return ZERO; 	
					} 
<GENERIC_IMAGE_DECL>"Pixel"		{
					strcpy(yylval.elem.string, DATATYPE_STR);
					yylval.elem.dtype = TYPE_CHANNEL;
					return(VectorChan);
					}
<GENERIC_IMAGE_DECL>"Channel"		{
    					strcpy(yylval.elem.string, DATATYPE_STR);
					yylval.elem.dtype = TYPE_CHANNEL;
					return(Chan);
					}

<GENERIC_IMAGE_DECL>{integer}		{ 
            				/* found an int */
					strcpy(yylval.elem.string, yytext);
                			yylval.elem.type = TYPE_SCALER;
					yylval.elem.dtype = TYPE_INT;
            				return INT;
					}
<GENERIC_IMAGE_DECL>{float} 		{ 
          				/* found a float */
          				strcpy(yylval.elem.string, yytext);
                			yylval.elem.type = TYPE_SCALER;
                			yylval.elem.dtype = TYPE_FLOAT; 
					return FLOAT;
        				}
<GENERIC_IMAGE_BLOCK>{integer}		{ 
            				/* found an int */
					strcpy(yylval.elem.string, yytext);
                			yylval.elem.type = TYPE_SCALER;
					yylval.elem.dtype = TYPE_INT;
            				return INT;
          				} 
<GENERIC_IMAGE_BLOCK>{float} 		{ 
          				/* found a float */
          				strcpy(yylval.elem.string, yytext);
                			yylval.elem.type = TYPE_SCALER;
                			yylval.elem.dtype = TYPE_FLOAT; 
					return FLOAT;
        				}

<GENERIC_IMAGE_DECL>"/*"		{ECHO; PREV=0; BEGIN COMMENT;}
<GENERIC_IMAGE_BLOCK>"/*"		{ECHO; PREV=1; BEGIN COMMENT;}
<COMMENT>"*/"				{
    					ECHO; 
					if (PREV) BEGIN GENERIC_IMAGE_BLOCK; 
    					else BEGIN GENERIC_IMAGE_DECL;
					} 

<GENERIC_IMAGE_DECL>"(color,alpha,has_alpha)"	return(COLOR_MAYBE_ALPHA);
<GENERIC_IMAGE_DECL>"(color,alpha)"		return(COLOR_ALPHA);
<GENERIC_IMAGE_DECL>"(color)"			return(COLOR);

<GENERIC_IMAGE_BLOCK>"dX"		return(ITERATOR_X);
<GENERIC_IMAGE_BLOCK>"dXY"            	return(ITERATOR_XY);

<GENERIC_IMAGE_BLOCK>"MAX"		{	
    					strcpy (yylval.tok.string, yytext); 
    					return (MIN_MAX);
					}
<GENERIC_IMAGE_BLOCK>"MIN"		{	
    					strcpy (yylval.tok.string, yytext); 
    					return (MIN_MAX);
					}
<GENERIC_IMAGE_BLOCK>"ABS"		return(ABS); 
<GENERIC_IMAGE_BLOCK>"CHANNEL_CLAMP"	return(CHANNEL_CLAMP);
<GENERIC_IMAGE_BLOCK>"WP_CLAMP"    	return(WP_CLAMP);


<GENERIC_IMAGE_BLOCK>"+"		return(PLUS);
<GENERIC_IMAGE_BLOCK>"-"		return(MINUS);
<GENERIC_IMAGE_DECL>"*"			return(TIMES);
<GENERIC_IMAGE_BLOCK>"*"		return(TIMES);
<GENERIC_IMAGE_BLOCK>"/"		return(DIVIDE);
<GENERIC_IMAGE_BLOCK>"^"		return(POWER);
<GENERIC_IMAGE_BLOCK>"("		return(LT_PARENTHESIS);
<GENERIC_IMAGE_BLOCK>")"		return(RT_PARENTHESIS);
<GENERIC_IMAGE_BLOCK>"{"		{SCOPE ++; return(LT_CURLY);}
<GENERIC_IMAGE_BLOCK>"}"		{SCOPE --; rm_varibles (SCOPE); return(RT_CURLY); } 
<GENERIC_IMAGE_DECL>"{"			{SCOPE ++; ECHO;}
<GENERIC_IMAGE_DECL>"}"			{SCOPE --; rm_varibles (SCOPE); ECHO;} 
"{"					{SCOPE ++; ECHO;}
"}"					{SCOPE --; rm_varibles (SCOPE); ECHO;} 
<GENERIC_IMAGE_DECL>"["			return(LT_SQUARE);
<GENERIC_IMAGE_BLOCK>"["		return(LT_SQUARE);
<GENERIC_IMAGE_DECL>"]"			return(RT_SQUARE); 
<GENERIC_IMAGE_BLOCK>"]"		return(RT_SQUARE); 
<GENERIC_IMAGE_DECL>"="			return(EQUAL); 
<GENERIC_IMAGE_BLOCK>"="		return(EQUAL); 
<GENERIC_IMAGE_BLOCK>"+="		return(PLUS_EQUAL); 
<GENERIC_IMAGE_BLOCK>"-="		return(MINUS_EQUAL); 
<GENERIC_IMAGE_BLOCK>"*="		return(TIMES_EQUAL); 
<GENERIC_IMAGE_BLOCK>"/="		return(DIVIDE_EQUAL); 
<GENERIC_IMAGE_BLOCK>"&&"		{
    					strcpy (yylval.tok.string, yytext); 
    					return (COMPARE);
					}
<GENERIC_IMAGE_BLOCK>"||"		{
    					strcpy (yylval.tok.string, yytext); 
    					return (COMPARE);
					}
<GENERIC_IMAGE_BLOCK>"=="		{
    					strcpy (yylval.tok.string, yytext); 
    					return (COMPARE);
					}
<GENERIC_IMAGE_BLOCK>"!="		{
    					strcpy (yylval.tok.string, yytext); 
    					return (COMPARE);
					}
<GENERIC_IMAGE_BLOCK>"<"		{
    					strcpy (yylval.tok.string, yytext); 
    					return (COMPARE);
					}
<GENERIC_IMAGE_BLOCK>">"		{
    					strcpy (yylval.tok.string, yytext); 
    					return (COMPARE);
					}
<GENERIC_IMAGE_BLOCK>"<="		{
    					strcpy (yylval.tok.string, yytext); 
    					return (COMPARE);
					}
<GENERIC_IMAGE_BLOCK>">="		{	
    					strcpy (yylval.tok.string, yytext); 
    					return (COMPARE);
					}
<GENERIC_IMAGE_BLOCK>"!"		return(NOT);
<GENERIC_IMAGE_BLOCK>"++"		{
    					strcpy (yylval.tok.string, yytext); 
    					return (ADD_SUB);
					}
<GENERIC_IMAGE_BLOCK>"--"		{
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
<GENERIC_IMAGE_BLOCK>{Name}  		{
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

<GENERIC_IMAGE_BLOCK>{name}  		{
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
<DATA_TYPE>{name}			{
    					strcpy (yylval.tok.string, yytext);
					return DT_NAME;
					}
<DATA_TYPE>{space}{string}			{
    					strcpy (yylval.tok.string, yytext);
					return DT_STRING;
					}


{indent}"$" 				return 0; 
<GENERIC_IMAGE_DECL>.			return yytext[0];
<GENERIC_IMAGE_BLOCK>.			return yytext[0];
<COMMENT>.				ECHO;
<DATA_TYPE>.				; 
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

void 
print_info (char *line)
{
#if DEBUG
  printf ("%s", line); 
#endif 
}
