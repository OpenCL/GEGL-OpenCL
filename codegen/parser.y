%{
#ifndef _PARSER_Y_
#define _PARSER_Y_
#endif

#include "common.h"
#include <string.h> 
#include <stdlib.h>

void do_op_one (elem_t *dest, FUNCTION op);
void do_op_two (elem_t *dest, elem_t src, FUNCTION op);
void do_op_three (elem_t *dest, elem_t src1, elem_t src2, FUNCTION op);
void print_value (elem_t *dest, elem_t src);
void print_stuff (elem_t *dest, elem_t src);
elem_t* get_sym (char *sym);
void set_dtype (elem_t e, DATA_TYPE dtype);
void set_type (elem_t e, DATA_TYPE dtype);
void read_data_types (char *data_type);
int yyerror (char *s); 
    
#define NSYMS 20          /* maximum number of symbols */
elem_t  symtab[NSYMS];
int     cur_nsyms=0;

keyword_t keyword_tab[] = {
{"break",      BREAK},
{"case",       CASE},
{"const",      CONST},
{"continue",   CONTINUE},
{"default",    DEFAULT},
{"do",         DO},
{"else",       ELSE},
{"for",        FOR},
{"gchar",      GCHAR},
{"gfloat",     GFLOAT},
{"gint",       GINT},
{"goto",       GOTO},
{"guchar",     GUCHAR},
{"guint8",     GUINT8},
{"guint16",    GUINT16},
{"gshort",     GSHORT},
{"if",         IF},
{"include",    INCLUDE}, 
{"long",       LONG},
{"real",       REAL},
{"register",   REGISTER},
{"return",     RETURN},
{"signed",     SIGNED},
{"sizeof",     SIZEOF},
{"static",     STATIC},
{"struct",     STRUCT},
{"switch",     SWITCH},
{"typedef",    TYPEDEF},
{"union",      UNION},
{"unsigned",   UNSIGNED},
{"void",       VOID}, 
{"volatile",   VOLATILE},
{"while",      WHILE},
{"",		-1}
  };

%}

%union
{
  elem_t  elem;
  node_t  node; 
}


%token 	<elem> NAME
%token	<elem> FLOAT
%token	<elem> INT
%token  <elem> WP 
%token  <elem> ZERO_CHAN  
%token  <elem> ColorAlphaChan
%token  <elem> ColorChan
%token  <elem> Chan
%token  <elem> FloatChan 
%token  <elem> INDENT
%token  <elem> POUND

/* keywords */
%token	BREAK  CASE  CONST  CONTINUE  DEFAULT  DO	
%token 	ELSE  FOR  GCHAR  GFLOAT  GINT  GOTO  GUCHAR
%token  GUINT8  GUINT16  GSHORT  IF  INCLUDE LONG  REAL  REGISTER
%token  RETURN  SIGNED  SIZEOF  STATIC  STRUCT  SWITCH  TYPEDEF
%token  UNION  UNSIGNED  VOID  VOLATILE  WHILE

/* operations */ 
%token	MAX  MIN  ABS  CHAN_CLAMP  WP_CLAMP
%token  PLUS  MINUS  TIMES  DIVIDE  POWER  LT_PARENTHESIS RT_PARENTHESIS
%token  LT_CURLY  RT_CURLY EQUAL PLUS_EQUAL MINUS_EQUAL TIMES_EQUAL DIVIDE_EQUAL
%token  AND OR EQ NOT_EQ SMALLER GREATER SMALLER_EQ GREATER_EQ NOT ADD SUBTRACT  

%left 		PLUS	MINUS
%left 		TIMES	DIVIDE
%right		EQUAL	PLUS_EQUAL      MINUS_EQUAL     TIMES_EQUAL     DIVIDE_EQUAL 
%right		POWER
%nonassoc	NEG

%type   <elem> Expression
%type   <elem> Definition
%type   <elem> FloatChan_List
%type   <elem> Chan_List
%type   <elem> ColorAlphaChan_List
%type   <elem> ColorChan_List
%type   <elem> GINT_List
%type   <elem> PoundInclDef
%type	<elem> Arguments

%start	Input

%%

Input:	
	
	| Input	Line
	;

Line:
	  ;
	| "\n" 				
		{ 
		printf("\n"); 
		}
	| VOID				
		{ 
		printf("void "); 
		}
	| POUND PoundInclDef 		
		{ 
		printf("#%s ", $2.string); 
		}
	| INDENT			
		{ 
		printf("%s", $1.string); 
		} 
	| LT_CURLY			
		{ 
		printf("{"); 
		}
	| RT_CURLY                      
		{ 
		printf("}"); 
		} 
	| BREAK ';'  			
		{ 
		printf("break;"); 
		} 			
	| CASE NAME ':'  		
		{ 
		printf("case %s:", $2.string); 
		}
       	| DEFAULT ':' 	 		
		{ 
		printf("default:"); 
		}
	| ELSE 				
		{ 
		printf("else "); 
		}
	| FOR LT_PARENTHESIS Expression ';' Expression ';' Expression RT_PARENTHESIS
		{ 
		printf("for (%s; %s; %s)", $3.string, $5.string, $7.string); 
		}
	| IF LT_PARENTHESIS Expression RT_PARENTHESIS
                { 
		printf("if (%s)", $3.string); 
		}	
 	| RETURN Expression ';'		
		{ 
		printf("return (%s)", $2.string); 
		}	
	| SWITCH LT_PARENTHESIS Expression RT_PARENTHESIS
                { 
		printf("switch (%s)", $3.string); 
		}
	| WHILE LT_PARENTHESIS Expression RT_PARENTHESIS
                { 
		printf("while (%s)", $3.string); 
		} 	
	| Definition ';' 		
		{ 
		printf("%s;", $1.string); 
		} 
	| NAME EQUAL Expression ';'  	
		{ 
		do_op_three (&$1, $1, $3, OP_EQUAL); 
		printf("%s;", $1.string); 
		} 
	| NAME PLUS_EQUAL Expression ';'  	
		{ 
		elem_t tmp; do_op_three (&tmp, $1, $3, OP_PLUS); 
		do_op_three (&$1, $1, tmp, OP_EQUAL); 
		printf("%s;", $1.string); 
		} 
	| NAME MINUS_EQUAL Expression ';'  	
		{ 
		elem_t tmp; 
		do_op_three (&tmp, $1, $3, OP_MINUS);
		do_op_three (&$1, $1, tmp, OP_EQUAL); 
		printf("%s;", $1.string); 
		} 
	| NAME TIMES_EQUAL Expression ';'  	
		{ 
		elem_t tmp; 
		do_op_three (&tmp, $1, $3, OP_TIMES);
		do_op_three (&$1, $1, tmp, OP_EQUAL); 
		printf("%s;", $1.string); 
		} 
	| NAME DIVIDE_EQUAL Expression ';'  	
		{ 
		elem_t tmp; 
		do_op_three (&tmp, $1, $3, OP_DIVIDE);
		do_op_three (&$1, $1, tmp, OP_EQUAL); 
		printf("%s;", $1.string); 
		} 
	| Expression ';'   		
		{ 
		printf("%s;", $1.string); 
		}
	| IF LT_PARENTHESIS Expression RT_PARENTHESIS
		{ 
		printf("if (%s)", $3.string); 
		}
	| NAME LT_PARENTHESIS Arguments RT_PARENTHESIS
       		{ 
		printf("%s (%s)", $1.string, $3.string); 
		}	
	;

	
PoundInclDef:
	  INCLUDE '"' NAME '.' NAME '"' 
		{ 
		char tmp[256]; 
		$$=$3; 
		sprintf (tmp,"include \"%s.%s\" ", $3.string, 
	      	$5.string);
                strcpy($$.string, tmp); 
		}
	| INCLUDE '<' NAME '.' NAME '>' 
		{ 
		char tmp[256]; 
		$$=$3; 
		sprintf (tmp,"include <%s.%s> ", $3.string, $5.string);
	        strcpy($$.string, tmp); 
		}
	;

Expression:
	  Expression PLUS Expression	
		{ 
		$$=$1; 
		do_op_three (&$$, $1, $3, OP_PLUS); 
		}
	| Expression MINUS Expression	
		{ 
		$$=$1; 
		do_op_three (&$$, $1, $3, OP_MINUS); 
		}
	| Expression TIMES Expression	
		{ 
		$$=$1; 
		do_op_three (&$$, $1, $3, OP_TIMES); 
		}
	| Expression DIVIDE Expression  
		{ 
		$$=$1; 
		do_op_three (&$$, $1, $3, OP_DIVIDE); 
		} 	 
	| Expression AND Expression  	
		{ 
		$$=$1; 
		do_op_three (&$$, $1, $3, OP_AND); 
		} 	 
	| Expression OR Expression  	
		{ 
		$$=$1; 
		do_op_three (&$$, $1, $3, OP_OR); 
		} 	 
	| Expression EQUAL Expression   
		{ 
		$$=$1; 
		do_op_three (&$$, $1, $3, OP_EQUAL); 
		}
	| MINUS Expression %prec NEG	
		{ 
		$$=$2; 
		do_op_two (&$$, $2, OP_NEG); 
		} 
	| LT_PARENTHESIS Expression RT_PARENTHESIS
		{ 
		$$=$2; 
		do_op_two (&$$, $2, OP_PARENTHESIS);
		} 
	| WP_CLAMP LT_PARENTHESIS Expression RT_PARENTHESIS
		{ 
		$$=$3; 
		do_op_two (&$$, $3, OP_WP_CLAMP); 
		}
	| CHAN_CLAMP LT_PARENTHESIS Expression RT_PARENTHESIS
                { 
		$$=$3; 
		do_op_two (&$$, $3, OP_CHAN_CLAMP); 
		} 
	| MIN LT_PARENTHESIS Expression ',' Expression RT_PARENTHESIS
                { 
		$$=$3; 
		do_op_three (&$$, $3, $5, OP_MIN); 
		}
	| MAX LT_PARENTHESIS Expression ',' Expression RT_PARENTHESIS
                { 
		$$=$3; 
		do_op_three (&$$, $3, $5, OP_MAX); 
		}
	| ABS LT_PARENTHESIS Expression RT_PARENTHESIS
                { 
		$$=$3; 
		do_op_two (&$$, $3, OP_ABS); 
		} 
	| Expression EQ Expression	
		{ 
		char tmp[256]; 
		$$=$3; 
		sprintf (tmp,"%s == %s", $1.string, $3.string);
                strcpy($$.string, tmp); 
		}
	| Expression NOT_EQ Expression	
		{ 
		char tmp[256]; 
		$$=$3; 
		sprintf (tmp,"%s != %s", $1.string, $3.string); 
	        strcpy($$.string, tmp); 
		}
	| Expression SMALLER Expression	
		{ 
		char tmp[256]; 
		$$=$3; 
		sprintf (tmp,"%s < %s", $1.string, $3.string); 
                strcpy($$.string, tmp); 
		}
	| Expression GREATER Expression	
		{ 
		char tmp[256]; 
		$$=$3; 
		sprintf (tmp,"%s > %s", $1.string, $3.string); 
	        strcpy($$.string, tmp); 
		}
	| Expression SMALLER_EQ Expression	
		{ 
		char tmp[256]; 
		$$=$3; 
		sprintf (tmp,"%s <= %s", $1.string, $3.string); 
	  	strcpy($$.string, tmp); 
		}
	| Expression GREATER_EQ Expression	
		{ 
		char tmp[256]; 
		$$=$3; 
		sprintf (tmp,"%s >= %s", $1.string, $3.string);
	        strcpy($$.string, tmp);	
		}
	| NOT Expression		
		{ 
		char tmp[256]; 
		$$=$2; 
		sprintf (tmp,"!%s", $2.string);
	        strcpy($$.string, tmp); 
		}
	| Expression ADD		
		{ 
		char tmp[256]; 
		$$=$1; 
		sprintf (tmp,"%s++", $1.string);
	        strcpy($$.string, tmp); 
		} 
	| Expression SUBTRACT		
		{ 
		char tmp[256]; 
		$$=$1; 
		sprintf (tmp,"%s--", $1.string);
	        strcpy($$.string, tmp); 
		} 
	| NAME				
		{ 
		$$=$1; 
		print_value(&$$, $1); 
		}
	| FLOAT				
		{ 
		$$=$1; 
		print_stuff(&$$, $1); 
		}
	| INT				
		{ 
		$$=$1; 
		print_stuff(&$$, $1); 
		} 
	| WP				
		{ 
		$$=$1; 
		print_stuff(&$$, $1); 
		}
	| ZERO_CHAN			
		{ 
		$$=$1; 
		print_stuff(&$$, $1); 
		} 
	;

Arguments:
	  Arguments ',' Arguments	
		{ 
		char tmp[256]; 
		$$=$3; 
		sprintf (tmp," %s, %s", $1.string, $3.string);
		strcpy($$.string, tmp); 
		}
	| GCHAR TIMES NAME		
		{ 
		char tmp[256]; 
		$$=$3; 
		sprintf (tmp,"gchar *%s", $3.string); 
	  	strcpy($$.string, tmp); 
		}
       	;
	
Definition:
	  FloatChan FloatChan_List 	
		{ 
		char tmp[256]; 
		$$=$2; 
		sprintf (tmp,"%s %s", $1.string, $2.string); 
	  	strcpy($$.string, tmp); 
		}
	| Chan Chan_List		
		{ 
		char tmp[256]; 
		$$=$2; 
		sprintf (tmp,"%s %s", $1.string, $2.string);
                strcpy($$.string, tmp); 
		}
	| ColorChan ColorChan_List
                {
                char tmp[256];
                $$=$2;
                sprintf (tmp,"%s %s", $1.string, $2.string);
                strcpy($$.string, tmp);
                }
	| ColorAlphaChan ColorAlphaChan_List
                {
                char tmp[256];
                $$=$2;
                sprintf (tmp,"%s %s", $1.string, $2.string);
                strcpy($$.string, tmp);
                }
	| GINT GINT_List		
		{ 
		char tmp[256]; 
		$$=$2; 
		sprintf (tmp,"gint %s", $2.string);
                strcpy($$.string, tmp); 
		}
	;

GINT_List:
	 NAME ',' GINT_List             
		{ 
		char tmp[256]; 
		$$=$3;
                set_dtype($1, TYPE_CHAN);
                set_type($1, TYPE_SCALER);
                sprintf (tmp, "%s, %s", $1.string, $3.string);
                strcpy($$.string, tmp); 
		}
        | NAME                          
		{ 
		set_dtype($1, TYPE_CHAN); set_type($1, TYPE_SCALER); 
	  	$$=$1; print_value (&$$, $1);
		}
        | NAME EQUAL FLOAT              
		{ 
		set_dtype($1, TYPE_CHAN);
                set_type($1, TYPE_SCALER);
                sprintf ($$.string, "%s=%s", $1.string, $3.string);
                $$.dtype = $1.dtype; 
		}
        | NAME EQUAL INT                
		{ 
		set_dtype($1, TYPE_CHAN);
                set_type($1, TYPE_SCALER);
                sprintf ($$.string, "%s=%s", $1.string, $3.string);
                $$.dtype = $1.dtype; }
        | NAME EQUAL FLOAT ',' GINT_List
                { 
		char tmp[256]; 
		$$=$5;
                set_dtype($1, TYPE_CHAN);
                set_type($1, TYPE_SCALER);
                sprintf (tmp, "%s=%s, %s", $1.string, $3.string, $5.string);
                strcpy($$.string, tmp); 
		}
        | NAME EQUAL INT ',' GINT_List
                { 
		char tmp[256]; 
		$$=$5;
                set_dtype($1, TYPE_CHAN);
                set_type($1, TYPE_SCALER);
                sprintf (tmp, "%s=%s, %s", $1.string, $3.string, $5.string);
                strcpy($$.string, tmp); 
		}
        ;

Chan_List:
          NAME ',' Chan_List            
		{ 
		char tmp[256]; 
		$$=$3;
                set_dtype($1, TYPE_CHAN);
                set_type($1, TYPE_VECTOR);
                sprintf (tmp, "%s, %s", $1.string, $3.string);
                strcpy($$.string, tmp); 
		}
        | NAME                          
		{ 
		set_dtype($1, TYPE_CHAN); 
		set_type($1, TYPE_VECTOR);
	  	$$=$1; 
		print_value (&$$, $1);
		}
        | NAME EQUAL FLOAT              
		{ 
		set_dtype($1, TYPE_CHAN);
                set_type($1, TYPE_VECTOR);
                sprintf ($$.string, "%s=%s", $1.string, $3.string);
                $$.dtype = $1.dtype; 
		}
        | NAME EQUAL INT                
		{ 
		set_dtype($1, TYPE_CHAN);
                set_type($1, TYPE_VECTOR);
                sprintf ($$.string, "%s=%s", $1.string, $3.string);
                $$.dtype = $1.dtype; 
		}
        | NAME EQUAL FLOAT ',' Chan_List
                { 
		char tmp[256]; 
		$$=$5;
                set_dtype($1, TYPE_CHAN);
                set_type($1, TYPE_VECTOR);
                sprintf (tmp, "%s=%s, %s", $1.string, $3.string, $5.string);
                strcpy($$.string, tmp); 
		}
        | NAME EQUAL INT ',' Chan_List
                { 
		char tmp[256]; 
		$$=$5;
                set_dtype($1, TYPE_CHAN);
                set_type($1, TYPE_VECTOR);
                sprintf (tmp, "%s=%s, %s", $1.string, $3.string, $5.string);
                strcpy($$.string, tmp); 
		}
        ;

ColorChan_List:
          NAME ',' ColorChan_List            
		{ 
		char tmp[256]; 
		$$=$3;
                set_dtype($1, TYPE_CHAN);
                set_type($1, TYPE_C_VECTOR);
                sprintf (tmp, "%s, %s", $1.string, $3.string);
                strcpy($$.string, tmp); 
		}
        | NAME                          
		{ 
		set_dtype($1, TYPE_CHAN); 
		set_type($1, TYPE_C_VECTOR);
	  	$$=$1; 
		print_value (&$$, $1);
		}
        | NAME EQUAL FLOAT              
		{ 
		set_dtype($1, TYPE_CHAN);
                set_type($1, TYPE_C_VECTOR);
                sprintf ($$.string, "%s=%s", $1.string, $3.string);
                $$.dtype = $1.dtype; 
		}
        | NAME EQUAL INT                
		{ 
		set_dtype($1, TYPE_CHAN);
                set_type($1, TYPE_C_VECTOR);
                sprintf ($$.string, "%s=%s", $1.string, $3.string);
                $$.dtype = $1.dtype; 
		}
        | NAME EQUAL FLOAT ',' ColorChan_List
                { 
		char tmp[256]; 
		$$=$5;
                set_dtype($1, TYPE_CHAN);
                set_type($1, TYPE_C_VECTOR);
                sprintf (tmp, "%s=%s, %s", $1.string, $3.string, $5.string);
                strcpy($$.string, tmp); 
		}
        | NAME EQUAL INT ',' ColorChan_List
                { 
		char tmp[256]; 
		$$=$5;
                set_dtype($1, TYPE_CHAN);
                set_type($1, TYPE_C_VECTOR);
                sprintf (tmp, "%s=%s, %s", $1.string, $3.string, $5.string);
                strcpy($$.string, tmp); 
		}
        ;
ColorAlphaChan_List:
          NAME ',' ColorAlphaChan_List            
		{ 
		char tmp[256]; 
		$$=$3;
                set_dtype($1, TYPE_CHAN);
                set_type($1, TYPE_CA_VECTOR);
                sprintf (tmp, "%s, %s", $1.string, $3.string);
                strcpy($$.string, tmp); 
		}
        | NAME                          
		{ 
		set_dtype($1, TYPE_CHAN); 
		set_type($1, TYPE_CA_VECTOR);
	  	$$=$1; 
		print_value (&$$, $1);
		}
        | NAME EQUAL FLOAT              
		{ 
		set_dtype($1, TYPE_CHAN);
                set_type($1, TYPE_CA_VECTOR);
                sprintf ($$.string, "%s=%s", $1.string, $3.string);
                $$.dtype = $1.dtype; 
		}
        | NAME EQUAL INT                
		{ 
		set_dtype($1, TYPE_CHAN);
                set_type($1, TYPE_CA_VECTOR);
                sprintf ($$.string, "%s=%s", $1.string, $3.string);
                $$.dtype = $1.dtype; 
		}
        | NAME EQUAL FLOAT ',' ColorAlphaChan_List
                { 
		char tmp[256]; 
		$$=$5;
                set_dtype($1, TYPE_CHAN);
                set_type($1, TYPE_CA_VECTOR);
                sprintf (tmp, "%s=%s, %s", $1.string, $3.string, $5.string);
                strcpy($$.string, tmp); 
		}
        | NAME EQUAL INT ',' ColorAlphaChan_List
                { 
		char tmp[256]; 
		$$=$5;
                set_dtype($1, TYPE_CHAN);
                set_type($1, TYPE_CA_VECTOR);
                sprintf (tmp, "%s=%s, %s", $1.string, $3.string, $5.string);
                strcpy($$.string, tmp); 
		}
        ;


FloatChan_List:
	  NAME ',' FloatChan_List	
		{ 
		char tmp[256]; 
		$$=$3; 
	  	set_dtype($1, TYPE_FLOAT); 
	  	set_type($1, TYPE_FLOAT); 
		sprintf (tmp, "%s, %s", $1.string, $3.string);
		strcpy($$.string, tmp); 
		}	
	| NAME  			
		{ 
		set_dtype($1, TYPE_FLOAT); 
		set_type($1, TYPE_FLOAT); 
	  	$$=$1; 
		print_value (&$$, $1);
		}
	| NAME EQUAL FLOAT 		
		{ 
		set_dtype($1, TYPE_FLOAT); 
	  	set_type($1, TYPE_FLOAT); 
	  	sprintf ($$.string, "%s=%s", $1.string, $3.string);
		$$.dtype = $1.dtype; 
		}
	| NAME EQUAL INT		
		{ 
		set_dtype($1, TYPE_FLOAT); 
		set_type($1, TYPE_FLOAT); 
		sprintf ($$.string, "%s=%s", $1.string, $3.string);
		$$.dtype = $1.dtype; 
		}
	| NAME EQUAL FLOAT ',' FloatChan_List
					
		{ 
		char tmp[256]; $$=$5;
		set_dtype($1, TYPE_FLOAT);
		set_type($1, TYPE_FLOAT); 
		sprintf (tmp, "%s=%s, %s", $1.string, $3.string, $5.string);
		strcpy($$.string, tmp); 
		}
	| NAME EQUAL INT ',' FloatChan_List
		{ 
		char tmp[256]; $$=$5;
		set_dtype($1, TYPE_FLOAT);
		set_type($1, TYPE_FLOAT); 
		sprintf (tmp, "%s=%s, %s", $1.string, $3.string, $5.string);
		strcpy($$.string, tmp); 
		}
	;

%%

#include <stdio.h>
void
print_stuff (elem_t *dest, elem_t src)
{
  char tmp[256];
  sprintf (tmp, "%s", src.string);
  dest->dtype = src.dtype;
  strcpy (dest->string, tmp);
}

void
print_value (elem_t *dest, elem_t src)
{
  char tmp[256];
  sprintf (tmp, "%s", src.string);
  strcpy (dest->string, tmp);
  dest->dtype = get_sym(src.string)->dtype;
}

void
do_op_two (elem_t *dest, elem_t src, FUNCTION op)
{
  char tmp[256];
  switch (op)
  {
  case OP_NEG:
    sprintf (tmp, "-%s", src.string);
    break;
  case OP_CHAN_CLAMP:
    switch (src.dtype)
      {
      case TYPE_FLOAT:
	sprintf (tmp, "%s", src.string);
	break;
      case TYPE_CHAN:
	sprintf (tmp, "%s%s%s", _CHAN_CLAMP_PRE_, src.string, _CHAN_CLAMP_SUF_);  
	break; 
      }
    break; 
  case OP_WP_CLAMP:
    switch (src.dtype)
      {
      case TYPE_FLOAT:
	sprintf (tmp, "CLAMP (%s, %s, %s)", src.string, _MIN_CHAN_, _WP_);
	break;
      case TYPE_CHAN:
	sprintf (tmp, "%s%s%s", _WP_CLAMP_PRE_, src.string, _WP_CLAMP_SUF_);
	break;
      }
    break;
  case OP_PARENTHESIS:
    sprintf (tmp, "(%s)", src.string);
    break;
  case OP_ABS:
    sprintf (tmp, "ABS (%s)", src.string);
    break;
  default:
    break; 
  }
  strcpy (dest->string, tmp);
}

void
do_op_three (elem_t *dest, elem_t src1, elem_t src2, FUNCTION op)
{
  char tmp[256];
  switch (op)
    {
    case OP_PLUS:
      switch (src1.dtype)
	{
	case TYPE_FLOAT:
	  sprintf (tmp, "%s + %s", src1.string, src2.string);
	  break;
	case TYPE_CHAN:
	  sprintf (tmp, "%s%s + %s%s", _PLUS_PRE_, src1.string, src2.string, _PLUS_SUF_);
	  break;
	}
      break;
    case OP_MINUS:
      switch (src1.dtype)
	{
	case TYPE_FLOAT:
	  sprintf (tmp, "%s - %s", src1.string, src2.string);
	  break;
	case TYPE_CHAN:
	  sprintf (tmp, "%s%s - %s%s", _MINUS_PRE_, src1.string, src2.string, _MINUS_SUF_);
	  break;
	}
      break;
    case OP_TIMES:
      switch (src1.dtype|src2.dtype)
	{
	case TYPE_FLOAT:
	  if (src1.type && src2.type)
	    sprintf (tmp, "%s%s * %s%s", _TIMES_VS_PRE_, src1.string, src2.string, _TIMES_VS_SUF_);
	  else
	    sprintf (tmp, "%s * %s * %s", src1.string, src2.string, _WP_NORM_);
	  break;
	case TYPE_CHAN:
	  if (src1.type && src2.type)
	    sprintf (tmp, "%s%s%s%s%s", _TIMES_VV_PRE_, src1.string, _TIMES_VV_MID_, src2.string, _TIMES_VV_SUF_);
	  else
	    sprintf (tmp, "%s%s * %s%s", _TIMES_VS_PRE_, src1.string, src2.string, _TIMES_VS_SUF_);
	  break;
	}
      break;
    case OP_DIVIDE:
      switch (src1.dtype)
	{
	case TYPE_FLOAT:
	  if (src1.type && src2.type)
	    sprintf (tmp, "%s * %s / %s", src1.string, _WP_, src2.string);
	  else
	    sprintf (tmp, "%s / %s", src1.string, src2.string);
	  break;
	case TYPE_CHAN:
	  if (src1.type && src2.type)
	    sprintf (tmp, "%s * %s / %s", src1.string, _WP_, src2.string);
	  else
	    sprintf (tmp, "%s / %s", src1.string, src2.string);
	  break;
	}
      break;
    case OP_AND:
      sprintf (tmp, "%s && %s", src1.string, src2.string);
      break;
    case OP_OR:
      sprintf (tmp, "%s || %s", src1.string, src2.string);
      break;
    case OP_MIN:
      sprintf (tmp, "MIN (%s,%s)", src1.string, src2.string);
      break;
    case OP_MAX:
      sprintf (tmp, "MAX (%s,%s)", src1.string, src2.string);
      break;
    case OP_EQUAL:
      switch (src1.dtype)
	{
	case TYPE_FLOAT:
	  switch (src2.dtype)
	    {
	    case TYPE_FLOAT:
	      sprintf (tmp, "%s = %s", src1.string, src2.string);
	      break;
	    case TYPE_CHAN:
	      sprintf (tmp, "%s = %s", src1.string, src2.string);         
	      break;
	    }   
	  break;
	case TYPE_CHAN:
	  switch (src2.dtype)
	    {
	    case TYPE_FLOAT:
	      sprintf (tmp, "%s = %s%s%s", src1.string, _EQUAL_CFC_PRE_, src2.string, _EQUAL_CFC_SUF_);
	      break;
	    case TYPE_CHAN:
	      sprintf (tmp, "%s = %s", src1.string, src2.string);          
	      break;
	    } 
	  break;
	}
      break; 

    default:
      break; 
    }
  
  dest->dtype = src1.dtype | src2.dtype; 
  dest->type = src1.type | src2.type;
  strcpy (dest->string, tmp);
}


void
set_dtype (elem_t e, DATA_TYPE dtype)
{
  int   i;
  char *s = strdup(e.string); 
  i = strlen(s);
  if (i>2)
  if (s[i-1] == 'a' && s[i-2] == '_')
  {
    s[i-1] = '\0';
    s[i-2] = '\0'; 
  } 

  for (i=0; i<cur_nsyms; i++)
  {
    /* is it already here? */
    if(!strcmp(symtab[i].string, e.string))
        symtab[i].dtype = dtype;
  }

  if (cur_nsyms == NSYMS)
  {
    yyerror("Too many symbols");
    exit(1);      /* cannot continue */
  }

   
}

void
set_type (elem_t e, SV_TYPE type)
{
  int   i;
  char *s = strdup(e.string); 
  i = strlen(s);
  if (i>2)
  if (s[i-1] == 'a' && s[i-2] == '_')
  {
    s[i-1] = '\0';
    s[i-2] = '\0'; 
  } 

  for (i=0; i<cur_nsyms; i++)
  {
    /* is it already here? */
    if(!strcmp(symtab[i].string, e.string))
        symtab[i].type = type;
  }

  if (cur_nsyms == NSYMS)
  {
    yyerror("Too many symbols");
    exit(1);      /* cannot continue */
  }

   
}

 
/* look up a symbol table entry, add if not present */
elem_t
add_sym (char *ss)
{
  int	i; 
  char *s = strdup(ss);

  i = strlen(s);
  if (i>2)
  if (s[i-1] == 'a' && s[i-2] == '_')
  {
    s[i-1] = '\0';
    s[i-2] = '\0'; 
  } 
  
  for (i=0; i<cur_nsyms; i++) 
  {
    /* is it already here? */
    if(!strcmp(symtab[i].string, s))
            return symtab[i];
   
  }

  if (cur_nsyms == NSYMS) 
  {
    yyerror("Too many symbols");
    exit(1);      /* cannot continue */
  }
  
  strcpy(symtab[cur_nsyms].string, s); 
  cur_nsyms++; 
  return symtab[cur_nsyms-1];

}

elem_t* 
get_sym (char *ss)
{

  int   i;
  char *s = strdup(ss);
 
  i = strlen(s);
  if (i>2)
  if (s[i-1] == 'a' && s[i-2] == '_')
  {
    s[i-1] = '\0';
    s[i-2] = '\0'; 
  } 

  for (i=0; i<cur_nsyms; i++)
  {
    /* is it already here? */
    if(!strcmp(symtab[i].string, s))
            return &(symtab[i]);
  
  }

  yyerror("Too many symbols");
  exit(1);      /* cannot continue */

} 

int
get_keyword (char *keywd)
{

  int i=0; 

  while (keyword_tab[i].token != -1)
    {
    if (!strcmp (keyword_tab[i].word, keywd))
      return keyword_tab[i].token;
    else
      i ++;
    }

  return -1; 
}
  

void
read_data_types (char *data_type)
{
/*
        UINT8
*/
   if (!strcmp (data_type, "UINT8"))
     { 
     _WP_         	= (char *) strdup ("255");
     _WP_NORM_      	= (char *) strdup ("(1.0/255.0)");
     _ColorAlphaChan_   = (char *) strdup ("guint8");
     _ColorChan_        = (char *) strdup ("guint8");
     _Chan_         	= (char *) strdup ("guint8");
     _FloatChan_    	= (char *) strdup ("float");
     _MIN_CHAN_     	= (char *) strdup ("0");

     _MAX_CHAN_    	= (char *) strdup ("255");
     _ZERO_CHAN_  	= (char *) strdup ("0");

     _CHAN_CLAMP_PRE_  	= (char *) strdup ("CLAMP (");
     _CHAN_CLAMP_SUF_	= (char *) strdup (", 0, 255)");

     _WP_CLAMP_PRE_   	= (char *) strdup ("CLAMP (");
     _WP_CLAMP_SUF_   	= (char *) strdup (", 0, 255)");

     _PLUS_PRE_      	= (char *) strdup ("");
     _PLUS_SUF_      	= (char *) strdup ("");

     _MINUS_PRE_    	= (char *) strdup ("");
     _MINUS_SUF_    	= (char *) strdup ("");

     _TIMES_VS_PRE_  	= (char *) strdup ("");
     _TIMES_VS_SUF_  	= (char *) strdup ("");

     _TIMES_VV_PRE_	= (char *) strdup ("INT_MULT(");
     _TIMES_VV_MID_	= (char *) strdup (",");
     _TIMES_VV_SUF_  	= (char *) strdup (")");

     _EQUAL_CFC_PRE_ 	= (char *) strdup ("ROUND(");
     _EQUAL_CFC_SUF_ 	= (char *) strdup (")");
   }
/*
        UINT16
*/
   if (!strcmp(data_type, "UINT16"))
     {

     _WP_          	= (char *) strdup ("4095");
     _WP_NORM_     	= (char *) strdup ("(1.0/4095.0)");
     _ColorAlphaChan_   = (char *) strdup ("guint16");
     _ColorChan_        = (char *) strdup ("guint16");
     _Chan_         	= (char *) strdup ("guint16");
     _FloatChan_    	= (char *) strdup ("float");
     _MIN_CHAN_      	= (char *) strdup ("0");
     _MAX_CHAN_        	= (char *) strdup ("65535");
     _ZERO_CHAN_      	= (char *) strdup ("0");

     _CHAN_CLAMP_PRE_ 	= (char *) strdup ("CLAMP (");
     _CHAN_CLAMP_SUF_  	= (char *) strdup (", 0, 65535)");

     _WP_CLAMP_PRE_   	= (char *) strdup ("CLAMP (");
     _WP_CLAMP_SUF_  	= (char *) strdup (", 0, 4095)");

     _PLUS_PRE_     	= (char *) strdup ("");
     _PLUS_SUF_     	= (char *) strdup ("");

     _MINUS_PRE_     	= (char *) strdup ("");
     _MINUS_SUF_        = (char *) strdup ("");

     _TIMES_VS_PRE_     = (char *) strdup ("");
     _TIMES_VS_SUF_     = (char *) strdup ("");

     _TIMES_VV_PRE_    	= (char *) strdup ("INT_MULT16(");
     _TIMES_VV_MID_    	= (char *) strdup (",");
     _TIMES_VV_SUF_    	= (char *) strdup (")");

     _EQUAL_CFC_PRE_   	= (char *) strdup ("ROUND(");
     _EQUAL_CFC_SUF_   	= (char *) strdup (")");
     }
     
/*
        FLOAT
*/
   if (!strcmp(data_type, "FLOAT"))
     {
     _WP_               = (char *) strdup ("1.0");
     _WP_NORM_          = (char *) strdup ("1.0");
     _ColorAlphaChan_   = (char *) strdup ("float");
     _ColorChan_        = (char *) strdup ("float");
     _Chan_             = (char *) strdup ("float");
     _FloatChan_        = (char *) strdup ("float");
     _MIN_CHAN_         = (char *) strdup ("0");
     _MAX_CHAN_         = (char *) strdup ("1.0");
     _ZERO_CHAN_        = (char *) strdup ("0");

     _CHAN_CLAMP_PRE_   = (char *) strdup ("");
     _CHAN_CLAMP_SUF_   = (char *) strdup ("");

     _WP_CLAMP_PRE_    	= (char *) strdup ("CLAMP (");
     _WP_CLAMP_SUF_     = (char *) strdup (", 0, 1)");

     _PLUS_PRE_         = (char *) strdup ("");
     _PLUS_SUF_         = (char *) strdup ("");

     _MINUS_PRE_       	= (char *) strdup ("");
     _MINUS_SUF_        = (char *) strdup ("");

     _TIMES_VS_PRE_     = (char *) strdup ("");
     _TIMES_VS_SUF_     = (char *) strdup ("");

     _TIMES_VV_PRE_     = (char *) strdup ("");
     _TIMES_VV_MID_     = (char *) strdup ("*");
     _TIMES_VV_SUF_     = (char *) strdup ("");

     _EQUAL_CFC_PRE_    = (char *) strdup ("");
     _EQUAL_CFC_SUF_    = (char *) strdup ("");
   }
}

int
yyerror (char *s)
{
  fprintf (stderr, "%s\n",s);
  return 0; 
}


int 
main (int argc, char **argv)
{
  yydebug = 1; 
  read_data_types (argv[1]);
  yyparse();

  return 0; 
}

