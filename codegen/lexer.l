%{

#include <string.h>
#include "common.h"
#include "parser.h" 
#include <math.h>
#include <ctype.h>

%}

%s			CODEGEN_DECL
%s			CODEGEN_BLOCK

white   		[ \t]+
indent			[\n][ ]*
digit   		[0-9]
integer 		{digit}+
float   		{digit}*\.{integer}
name    		[a-zA-Z][A-Z_a-z0-9]*

%%

"CODEGEN_DECL_BEGIN"		BEGIN CODEGEN_DECL;
				
"CODEGEN_BLOCK_BEGIN"		BEGIN CODEGEN_BLOCK;
"CODEGEN_IMAGE_DATA_INIT"	{}
"COLORSPACE"			printf ("%s", _COLORSPACE_);
"Colorspace"			printf ("%s", _Colorspace_);
"colorspace"                    printf ("%s", _colorspace_);
"DATATYPE"			printf ("%s", _DATATYPE_);
"Datatype"		        printf ("%s", _Datatype_);
"datatype"                      printf ("%s", _datatype_); 

<CODEGEN_DECL>{indent}          {
                            	strcpy(yylval.elem.string, yytext);
			        return INDENT;
			        }
<CODEGEN_BLOCK>{indent}         {
                                strcpy(yylval.elem.string, yytext);
				return INDENT;
				}

			    	 	
<CODEGEN_DECL>{white} 		; 
<CODEGEN_BLOCK>{white} 		; 

<CODEGEN_DECL>"CODEGEN_DECL_END" 	BEGIN 0;
<CODEGEN_BLOCK>"CODEGEN_BLOCK_END" 	BEGIN 0;

<CODEGEN_BLOCK>"WP"		{
				strcpy(yylval.elem.string, _WP_);
                		yylval.elem.dtype = TYPE_CHAN;
				yylval.elem.type = TYPE_SCALER;
				return(WP); 
				}
<CODEGEN_BLOCK>"ZERO_CHAN"	{
    				strcpy(yylval.elem.string, _ZERO_CHAN_);
				yylval.elem.dtype = TYPE_CHAN;
				yylval.elem.type = TYPE_SCALER;
				return(ZERO_CHAN);
				}
<CODEGEN_DECL>"VectorChan"	{
				strcpy(yylval.elem.string, _VectorChan_);
				yylval.elem.dtype = TYPE_CHAN;
				return(VectorChan);
				}
<CODEGEN_DECL>"Chan"		{
    				strcpy(yylval.elem.string, _Chan_);
				yylval.elem.dtype = TYPE_CHAN;
				return(Chan);
				}
<CODEGEN_DECL>"FloatChan"	{
				strcpy(yylval.elem.string, _FloatChan_);
				yylval.elem.dtype = TYPE_FLOAT;
				return(FloatChan);  
				} 

<CODEGEN_DECL>{integer}		{ 
            			/* found an int */
				strcpy(yylval.elem.string, yytext);
                		yylval.elem.type = TYPE_SCALER;
				yylval.elem.dtype = TYPE_CHAN;
            			return INT;
				}
<CODEGEN_DECL>{float} 		{ 
          			/* found a float */
          			strcpy(yylval.elem.string, yytext);
                		yylval.elem.type = TYPE_SCALER;
                		yylval.elem.dtype = TYPE_FLOAT; 
				return FLOAT;
        			}
<CODEGEN_BLOCK>{integer}	{ 
            			/* found an int */
				strcpy(yylval.elem.string, yytext);
                		yylval.elem.type = TYPE_SCALER;
				yylval.elem.dtype = TYPE_CHAN;
            			return INT;
          			} 
<CODEGEN_BLOCK>{float} 		{ 
          			/* found a float */
          			strcpy(yylval.elem.string, yytext);
                		yylval.elem.type = TYPE_SCALER;
                		yylval.elem.dtype = TYPE_FLOAT; 
				return FLOAT;
        			}

<CODEGEN_DECL>"(color,alpha)"	return(COLOR_ALPHA);
<CODEGEN_DECL>"(color)"		return(COLOR);

<CODEGEN_BLOCK>"MAX"		return(MAX);
<CODEGEN_BLOCK>"MIN"		return(MIN);
<CODEGEN_BLOCK>"ABS"		return(ABS); 
<CODEGEN_BLOCK>"CHAN_CLAMP"	return(CHAN_CLAMP);
<CODEGEN_BLOCK>"WP_CLAMP"    	return(WP_CLAMP);


<CODEGEN_BLOCK>"+"		return(PLUS);
<CODEGEN_BLOCK>"-"		return(MINUS);
<CODEGEN_DECL>"*"		return(TIMES);
<CODEGEN_BLOCK>"*"		return(TIMES);
<CODEGEN_BLOCK>"/"		return(DIVIDE);
<CODEGEN_BLOCK>"^"		return(POWER);
<CODEGEN_BLOCK>"("		return(LT_PARENTHESIS);
<CODEGEN_BLOCK>")"		return(RT_PARENTHESIS);
<CODEGEN_BLOCK>"{"		return(LT_CURLY);
<CODEGEN_BLOCK>"}"		return(RT_CURLY); 
<CODEGEN_DECL>"["		return(LT_SQUARE);
<CODEGEN_BLOCK>"["		return(LT_SQUARE);
<CODEGEN_DECL>"]"		return(RT_SQUARE); 
<CODEGEN_BLOCK>"]"		return(RT_SQUARE); 
<CODEGEN_DECL>"="		return(EQUAL); 
<CODEGEN_BLOCK>"="		return(EQUAL); 
<CODEGEN_BLOCK>"+="		return(PLUS_EQUAL); 
<CODEGEN_BLOCK>"-="		return(MINUS_EQUAL); 
<CODEGEN_BLOCK>"*="		return(TIMES_EQUAL); 
<CODEGEN_BLOCK>"/="		return(DIVIDE_EQUAL); 
<CODEGEN_BLOCK>"&&"		return(AND);
<CODEGEN_BLOCK>"||"		return(OR);
<CODEGEN_BLOCK>"=="		return(EQ);
<CODEGEN_BLOCK>"!="		return(NOT_EQ);
<CODEGEN_BLOCK>"<"		return(SMALLER);
<CODEGEN_BLOCK>">"		return(GREATER);
<CODEGEN_BLOCK>"<="		return(SMALLER_EQ);
<CODEGEN_BLOCK>">="		return(GREATER_EQ);
<CODEGEN_BLOCK>"!"		return(NOT);
<CODEGEN_BLOCK>"#"		return(POUND);
<CODEGEN_BLOCK>"++"		return(ADD);
<CODEGEN_BLOCK>"--"		return(SUBTRACT);

<CODEGEN_DECL>{name}  		{
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
<CODEGEN_BLOCK>{name}  		{
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

"$" 				return 0; 
<CODEGEN_DECL>.			return yytext[0];
<CODEGEN_BLOCK>.		return yytext[0];
.				ECHO;


%%


