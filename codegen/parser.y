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
void print_name (elem_t *dest, elem_t src, TYPE_DEF is_define);
void print_line (elem_t src);
elem_t* get_sym (char *sym);
void set_dtype (elem_t e, DATA_TYPE dtype);
void set_type (elem_t e, DATA_TYPE dtype);
void set_num (elem_t e, int n);
void read_data_types (char *data_type);
void read_color_space (char *color_space); 
int yyerror (char *s); 
    
#define NSYMS 20           /* maximum number of symbols */
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
%token  <elem> VectorChan
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
%token  LT_CURLY  RT_CURLY LT_SQUARE RT_SQUARE 
%token  EQUAL PLUS_EQUAL MINUS_EQUAL TIMES_EQUAL DIVIDE_EQUAL
%token  AND OR EQ NOT_EQ SMALLER GREATER SMALLER_EQ GREATER_EQ NOT ADD SUBTRACT  

%token  COLOR  COLOR_ALPHA 

%left 		PLUS	MINUS
%left 		TIMES	DIVIDE
%right		EQUAL	PLUS_EQUAL      MINUS_EQUAL     TIMES_EQUAL     DIVIDE_EQUAL 
%right		POWER
%nonassoc	NEG

%type   <elem> Expression
%type   <elem> Definition
%type   <elem> FloatChan_List
%type   <elem> Chan_List
%type   <elem> VectorChan_List
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
	| INDENT
		{
		printf("%s", $1.string); 
		}
	| VOID				
		{ 
		printf("void "); 
		}
	| POUND PoundInclDef 		
		{ 
		printf("#%s ", $2.string); 
		}
	| INDENT LT_CURLY			
		{ 
		printf("%s{", $1.string); 
		}
	| INDENT RT_CURLY                      
		{ 
		printf("%s}", $1.string); 
		} 
	| INDENT BREAK ';'  			
		{ 
		printf("%sbreak;", $1.string); 
		} 			
	| INDENT CASE NAME ':'  		
		{ 
		printf("%scase %s:", $1.string, $3.string); 
		}
       	| INDENT DEFAULT ':' 	 		
		{ 
		printf("%sdefault:", $1.string); 
		}
	| INDENT ELSE 				
		{ 
		printf("%selse ", $1.string); 
		}
	| INDENT FOR LT_PARENTHESIS Expression ';' Expression ';' Expression RT_PARENTHESIS
		{ 
		printf("%sfor (%s; %s; %s)", $1.string, $4.string, $6.string, $8.string); 
		}
	| INDENT IF LT_PARENTHESIS Expression RT_PARENTHESIS
                { 
		printf("%sif (%s)", $1.string, $4.string); 
		}	
 	| INDENT RETURN Expression ';'		
		{ 
		printf("%sreturn (%s);", $1.string, $3.string); 
		}	
	| INDENT SWITCH LT_PARENTHESIS Expression RT_PARENTHESIS
                { 
		printf("%sswitch (%s)", $1.string, $4.string); 
		}
	| INDENT WHILE LT_PARENTHESIS Expression RT_PARENTHESIS
                { 
		printf("%swhile (%s)", $1.string, $4.string); 
		} 	
	| INDENT Definition ';' 		
		{ 
		printf("%s%s;", $1.string, $2.string); 
		} 
	| INDENT NAME EQUAL Expression ';'  	
		{
	        char tmp[256];	
		print_name (&$2, $2, NOT_DEFINE); 
		do_op_three (&$2, $2, $4, OP_EQUAL); 
		sprintf (tmp, "%s%s;", $1.string, $2.string);   
		strcpy ($2.string, tmp); 
		print_line ($2); 
		} 
	| INDENT NAME PLUS_EQUAL Expression ';'  	
		{ 
		elem_t tmp; 
		do_op_three (&tmp, $2, $4, OP_PLUS); 
		print_name (&$2, $2, NOT_DEFINE);
		do_op_three (&$2, $2, tmp, OP_EQUAL); 
		sprintf ($1.string, "%s%s;", $1.string, $2.string);   
		print_line ($1); 
		} 
	| INDENT NAME MINUS_EQUAL Expression ';'  	
		{ 
		elem_t tmp; 
		do_op_three (&tmp, $2, $4, OP_PLUS); 
		print_name (&$2, $2, NOT_DEFINE);
		do_op_three (&$2, $2, tmp, OP_EQUAL); 
		sprintf ($1.string, "%s%s;", $1.string, $2.string);   
		print_line ($1); 
		} 
	| INDENT NAME TIMES_EQUAL Expression ';'  	
		{ 
		elem_t tmp; 
		do_op_three (&tmp, $2, $4, OP_PLUS); 
		print_name (&$2, $2, NOT_DEFINE);
		do_op_three (&$2, $2, tmp, OP_EQUAL); 
		sprintf ($1.string, "%s%s;", $1.string, $2.string);   
		print_line ($1); 
		} 
	| INDENT NAME DIVIDE_EQUAL Expression ';'  	
		{ 
		elem_t tmp; 
		do_op_three (&tmp, $2, $4, OP_PLUS); 
		print_name (&$2, $2, NOT_DEFINE);
		do_op_three (&$2, $2, tmp, OP_EQUAL); 
		sprintf ($1.string, "%s%s;", $1.string, $2.string);   
		print_line ($1); 
		} 
	| INDENT Expression ';'   		
		{ 
		sprintf ($1.string, "%s%s;", $1.string, $2.string);   
		print_line($1); 
		}
	| INDENT NAME LT_PARENTHESIS Arguments RT_PARENTHESIS
       		{ 
		printf("%s%s (%s)", $1.string, $2.string, $4.string); 
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
		print_name(&$$, $1, NOT_DEFINE); 
		}
	| FLOAT				
		{ 
		$$=$1; 
		print_value(&$$, $1); 
		}
	| INT				
		{ 
		$$=$1; 
		print_value(&$$, $1); 
		} 
	| WP				
		{ 
		$$=$1; 
		print_value(&$$, $1); 
		}
	| ZERO_CHAN			
		{ 
		$$=$1; 
		print_value(&$$, $1); 
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
	| VectorChan VectorChan_List
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
	 GINT_List ',' GINT_List             
		{ 
		char tmp[256]; 
		$$=$3;
                sprintf (tmp, "%s, %s", $1.string, $3.string);
                strcpy($$.string, tmp); 
		}
        | NAME                          
		{ 
		set_dtype($1, TYPE_CHAN); 
		set_type($1, TYPE_SCALER); 
	        set_num ($1, 1); 	
	  	$$=$1; print_name (&$$, $1, DEFINE);
		}
        | NAME EQUAL FLOAT              
		{ 
		set_dtype($1, TYPE_CHAN);
                set_type($1, TYPE_SCALER);
	        set_num ($1, 1); 	
                sprintf ($$.string, "%s=%s", $1.string, $3.string);
                $$.dtype = $1.dtype; 
		}
        | NAME EQUAL INT                
		{ 
		set_dtype($1, TYPE_CHAN);
                set_type($1, TYPE_SCALER);
	        set_num ($1, 1); 	
                sprintf ($$.string, "%s=%s", $1.string, $3.string);
                $$.dtype = $1.dtype; }
        ;

Chan_List:
          Chan_List ',' Chan_List            
		{ 
		char tmp[256]; 
		$$=$3;
                sprintf (tmp, "%s, %s", $1.string, $3.string);
                strcpy($$.string, tmp); 
		}
        | NAME                          
		{ 
		set_dtype($1, TYPE_CHAN); 
		set_type($1, TYPE_SCALER);
	        set_num ($1, 1); 	
	  	$$=$1; 
		print_name (&$$, $1, NOT_DEFINE);
		}
        | NAME EQUAL FLOAT              
		{ 
		set_dtype($1, TYPE_CHAN);
                set_type($1, TYPE_SCALER);
	        set_num ($1, 1); 	
                sprintf ($$.string, "%s=%s", $1.string, $3.string);
                $$.dtype = $1.dtype; 
		}
        | NAME EQUAL INT                
		{ 
		set_dtype($1, TYPE_CHAN);
                set_type($1, TYPE_SCALER);
	        set_num ($1, 1); 	
                sprintf ($$.string, "%s=%s", $1.string, $3.string);
                $$.dtype = $1.dtype; 
		}
        ;

VectorChan_List:
          VectorChan_List ',' VectorChan_List            
		{ 
		char tmp[256]; 
		$$=$3;
                sprintf (tmp, "%s, %s", $1.string, $3.string);
                strcpy($$.string, tmp); 
		}
        | NAME COLOR                         
		{ 
		set_dtype($1, TYPE_CHAN); 
		set_type($1, TYPE_C_VECTOR);
		set_num($1, _NUM_COLOR_CHAN_);
	  	$$=$1; 
		print_name (&$$, $1, DEFINE);
		}
	| NAME COLOR_ALPHA
		{
		set_dtype($1, TYPE_CHAN);
		set_type($1, TYPE_CA_VECTOR);
		set_num($1, _NUM_COLOR_CHAN_+1); 
		$$=$1;
		print_name (&$$, $1, DEFINE);
		}
        | NAME LT_SQUARE INT RT_SQUARE
		{
		set_dtype($1, TYPE_CHAN);
		set_type($1, TYPE_VECTOR);
		set_num($1, atoi ($3.string)); 
	        $$=$1;
		print_name (&$$, $1, DEFINE);
		}	
	;


FloatChan_List:
	  FloatChan_List ',' FloatChan_List	
		{ 
		char tmp[256]; 
		$$=$3; 
		sprintf (tmp, "%s, %s", $1.string, $3.string);
		strcpy($$.string, tmp); 
		}	
	| NAME  			
		{ 
		set_dtype ($1, TYPE_FLOAT); 
		set_type ($1, TYPE_SCALER);
	        set_num ($1, 1); 	
	  	$$=$1; 
		print_name (&$$, $1, DEFINE);
		}
	| NAME EQUAL FLOAT 		
		{ 
		set_dtype ($1, TYPE_FLOAT); 
	  	set_type ($1, TYPE_SCALER); 
	        set_num ($1, 1); 	
	  	sprintf ($$.string, "%s=%s", $1.string, $3.string);
		$$.dtype = $1.dtype; 
		}
	| NAME EQUAL INT		
		{ 
		set_dtype ($1, TYPE_FLOAT); 
		set_type ($1, TYPE_SCALER); 
	        set_num ($1, 1); 	
		sprintf ($$.string, "%s=%s", $1.string, $3.string);
		$$.dtype = $1.dtype; 
		}
	;

%%

#include <stdio.h>
void
print_name (elem_t *dest, elem_t src, TYPE_DEF is_define)
{
  char tmp[256];


  if (is_define && get_sym (src.string)->type == TYPE_C_VECTOR)
    {
    sprintf (tmp, "%s_c", get_sym (src.string)->string);
    dest->num = 3;
    }
  else if (is_define && get_sym (src.string)->type == TYPE_CA_VECTOR && 
      !strcmp (src.string, get_sym (src.string)->string))
    sprintf (tmp, "%s_ca", get_sym (src.string)->string);
  else if (is_define && get_sym (src.string)->type == TYPE_CA_VECTOR)
    {
    int l = strlen (src.string);
    if (src.string[l-1] == 'a' && src.string[l-2] == '_')
      {
      dest->num = 1;
      sprintf (tmp, "%s", src.string);
      }
    if (src.string[l-1] == 'c' && src.string[l-2] == '_')
      {
      dest->num = 3; 
      sprintf (tmp, "%s", src.string);
      }
    } 
  else if (is_define && get_sym (src.string)->type == TYPE_VECTOR)
    sprintf (tmp, "%s_v", src.string);
  else if (!is_define && get_sym (src.string)->type == TYPE_VECTOR)
    sprintf (tmp, "%s[%d]", src.string, get_sym (src.string)->num);
  else
    sprintf (tmp, "%s", src.string);

  dest->dtype = src.dtype;
  strcpy (dest->string, tmp);
}

void
print_line (elem_t src)
{

  int i,j,k=1;
  int l = strlen (src.string);
 
  if (src.type>1 || (src.type && src.num == _NUM_COLOR_CHAN_)) /* if it is a vector */ 
  for (i=0; i<k; i++)
    {
    for(j=0; j<l; j++)
      {
      if (j < l-2 && src.string[j] == '_' && src.string[j+1] == 'c' && src.string[j+2] == 'a')
	{
	printf("_%c", _NAME_COLOR_CHAN_[i]);
	k = _NUM_COLOR_CHAN_ + 1;
	j += 2;
	}
      else if (j < l-1 && src.string[j] == '_' && src.string[j+1] == 'c')
	{
	printf("_%c", _NAME_COLOR_CHAN_[i]);
	k = _NUM_COLOR_CHAN_; 	
	j++;
	}
      else if (j < l-1 && src.string[j] == '_' && src.string[j+1] == 'v')
	{
	printf("[%d]", i);
	k = _NUM_COLOR_CHAN_; 
	j++;
	}
      else
	{
	printf("%c", src.string[j]);
	}
      }
    }
  else if (src.type)
    for (i=0; i<src.num; i++)
      {
      for(j=0; j<l; j++)
	{
	if (j < l-1 && src.string[j] == '_' && src.string[j+1] == 'v')
	  {
	  printf("[%d]", i);
	  j++;
	  }
	else
	  {
	  printf("%c", src.string[j]); 
	  }
	}
      }
  else
    printf("%s", src.string); 
  
}

void
print_value (elem_t *dest, elem_t src)
{
  char tmp[256];
  sprintf (tmp, "%s", src.string);
  strcpy (dest->string, tmp);
  dest->dtype = src.dtype;
  dest->num = 1;

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
  
  /* error checking */
  if (src1.num != src2.num  && src1.num != 1 && src2.num != 1)
    {
    yyerror("ERROR: Error");
    exit(1); 
    }

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
  	
      /* error checking */
      if (src1.num != src2.num && !(src1.num > src2.num && src2.num == 1))
	{
	yyerror("ERROR: op_EQUAL");
	exit(1); 
	}
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
  dest->type  = (src1.type > src2.type)?src1.type:src2.type;
  dest->num   = (src1.num > src2.num)?src1.num:src2.num;
  strcpy (dest->string, tmp);
}


void
set_dtype (elem_t e, DATA_TYPE dtype)
{
  int   i;
  char *s = strdup(e.string); 
  i = strlen(s);
  if (i>2)
  if ((s[i-1] == 'a' || s[i-1] == 'c') && s[i-2] == '_')
  {
    s[i-1] = '\0';
    s[i-2] = '\0'; 
  } 

  for (i=0; i<cur_nsyms; i++)
  {
    /* is it already here? */
    if(!strcmp(symtab[i].string, s))
      {
        symtab[i].dtype = dtype;
  	return;
      }
  }

  if (cur_nsyms == NSYMS)
  {
    yyerror("==>DTYPE");
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
  if ((s[i-1] == 'a' || s[i-1] == 'c') && s[i-2] == '_')
  {
    s[i-1] = '\0';
    s[i-2] = '\0'; 
  } 

  for (i=0; i<cur_nsyms; i++)
  {
    /* is it already here? */
    if(!strcmp(symtab[i].string, s))
      {
        symtab[i].type = type;
	return;
      }
  }

  if (cur_nsyms == NSYMS)
  {
    yyerror("==>TYPE");
    exit(1);      /* cannot continue */
  }

   
}

void
set_num (elem_t e, int n)
{
  int   i;
      
  for (i=0; i<cur_nsyms; i++)
    {
    /* is it already here? */
    if(!strcmp(symtab[i].string, e.string))
      {
      symtab[i].num = n;
      return;
      }
    }
  
  if (cur_nsyms == NSYMS)
    {
    yyerror("==>TYPE");
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
  if ((s[i-1] == 'a' || s[i-1] == 'c') && s[i-2] == '_')
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
  if ((s[i-1] == 'a' || s[i-1] == 'c') && s[i-2] == '_')
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

  yyerror("==>GET");
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
     _VectorChan_       = (char *) strdup ("guint8");
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
     _VectorChan_       = (char *) strdup ("guint16");
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
     _VectorChan_       = (char *) strdup ("float");
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

void
read_color_space (char *color_space)
{

  int i;

  _NUM_COLOR_CHAN_ = strlen (color_space); 
  _NAME_COLOR_CHAN_ = (char*) malloc (sizeof(char) * (_NUM_COLOR_CHAN_ + 1));

  for (i=0; i<_NUM_COLOR_CHAN_; i++)
    {
    _NAME_COLOR_CHAN_[i] = color_space[i]; 
    }
  _NAME_COLOR_CHAN_[_NUM_COLOR_CHAN_] = 'a'; 
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
  read_color_space (argv[2]); 
  yyparse();

  return 0; 
}

