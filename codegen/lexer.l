%{
#ifndef _LEXER_C_
#define _LEXER_C_
#endif

#include <string.h>
#include "common.h"
#include "parser.h" 
#include <math.h>
#include <ctype.h>
int PREV;  
%}

%s			GENERIC_IMAGE_DECL
%s			GENERIC_IMAGE_BLOCK
%s			COMMENT 
white   		[ \t]+
indent			[\n][ ]*
digit   		[0-9]
integer 		{digit}+
float   		{digit}*\.{integer}
name    		[a-zA-Z][A-Z_a-z0-9]*


%%

"GENERIC_IMAGE_DECL_BEGIN"		BEGIN GENERIC_IMAGE_DECL;
				
"GENERIC_IMAGE_BLOCK_BEGIN"		BEGIN GENERIC_IMAGE_BLOCK;
{indent}"GENERIC_IMAGE_IMAGE_DATA_INIT" 	init_image_data (yylval.elem.string); 	

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

<GENERIC_IMAGE_DECL>"GENERIC_IMAGE_DECL_END" 	BEGIN 0;
<GENERIC_IMAGE_BLOCK>"GENERIC_IMAGE_BLOCK_END" 	BEGIN 0;

<GENERIC_IMAGE_BLOCK>"WP"		{
					strcpy(yylval.elem.string, _WP_);
                			yylval.elem.dtype = TYPE_CHAN;
					yylval.elem.type = TYPE_SCALER;
					return(WP); 
					}
<GENERIC_IMAGE_BLOCK>"ZERO_CHAN"	{
    					strcpy(yylval.elem.string, _ZERO_CHAN_);
					yylval.elem.dtype = TYPE_CHAN;
					yylval.elem.type = TYPE_SCALER;
					return(ZERO_CHAN);
					}
<GENERIC_IMAGE_BLOCK>"ZERO"		{
    					strcpy(yylval.elem.string, _ZERO_);
					yylval.elem.dtype = TYPE_CHAN;
			        	yylval.elem.type = TYPE_VECTOR;
					return ZERO; 	
					} 
<GENERIC_IMAGE_DECL>"VectorChan"	{
					strcpy(yylval.elem.string, _VectorChan_);
					yylval.elem.dtype = TYPE_CHAN;
					return(VectorChan);
					}
<GENERIC_IMAGE_DECL>"Chan"		{
    					strcpy(yylval.elem.string, _Chan_);
					yylval.elem.dtype = TYPE_CHAN;
					return(Chan);
					}
<GENERIC_IMAGE_DECL>"FloatChan"		{
					strcpy(yylval.elem.string, _FloatChan_);
					yylval.elem.dtype = TYPE_FLOAT;
					return(FloatChan);  
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
<COMMENT>"*/"				{ECHO; if (PREV) BEGIN GENERIC_IMAGE_BLOCK; else GENERIC_IMAGE_DECL;} 

<GENERIC_IMAGE_DECL>"(color,alpha)"	return(COLOR_ALPHA);
<GENERIC_IMAGE_DECL>"(color)"		return(COLOR);

<GENERIC_IMAGE_BLOCK>"dX"		return(ITERATOR_X);
<GENERIC_IMAGE_BLOCK>"dXY"            	return(ITERATOR_XY);

<GENERIC_IMAGE_BLOCK>"MAX"		return(MAX);
<GENERIC_IMAGE_BLOCK>"MIN"		return(MIN);
<GENERIC_IMAGE_BLOCK>"ABS"		return(ABS); 
<GENERIC_IMAGE_BLOCK>"CHAN_CLAMP"	return(CHAN_CLAMP);
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
<GENERIC_IMAGE_DECL>"{"			{SCOPE ++;}
<GENERIC_IMAGE_DECL>"}"			{SCOPE --; rm_varibles (SCOPE);} 
"{"					{SCOPE ++;}
"}"					{SCOPE --; rm_varibles (SCOPE);} 
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
<GENERIC_IMAGE_BLOCK>"&&"		return(AND);
<GENERIC_IMAGE_BLOCK>"||"		return(OR);
<GENERIC_IMAGE_BLOCK>"=="		return(EQ);
<GENERIC_IMAGE_BLOCK>"!="		return(NOT_EQ);
<GENERIC_IMAGE_BLOCK>"<"		return(SMALLER);
<GENERIC_IMAGE_BLOCK>">"		return(GREATER);
<GENERIC_IMAGE_BLOCK>"<="		return(SMALLER_EQ);
<GENERIC_IMAGE_BLOCK>">="		return(GREATER_EQ);
<GENERIC_IMAGE_BLOCK>"!"		return(NOT);
<GENERIC_IMAGE_BLOCK>"#"		return(POUND);
<GENERIC_IMAGE_BLOCK>"++"		return(ADD);
<GENERIC_IMAGE_BLOCK>"--"		return(SUBTRACT);

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

"$" 					return 0; 
<GENERIC_IMAGE_DECL>.			return yytext[0];
<GENERIC_IMAGE_BLOCK>.			return yytext[0];
<COMMENT>.				ECHO;
.					ECHO;


%%

