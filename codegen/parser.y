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
void print_repeat (elem_t *dest, elem_t src, char *string); 
void print_line (elem_t src);
void set_dtype (elem_t e, DATA_TYPE dtype);
void set_type (elem_t e, DATA_TYPE dtype);
void set_num (elem_t e, int n);
void read_data_types (char *data_type);
void read_color_space (char *color_space); 
void init_data_varible (char *s);

int yyerror (char *s); 
    
#define NSYMS 100           /* maximum number of symbols */
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
{"float",      FLOAT},
{"for",        FOR},
{"gchar",      GCHAR},
{"gfloat",     GFLOAT},
{"gint",       GINT},
{"goto",       GOTO},
{"gboolean",   GBOOLEAN}, 
{"guchar",     GUCHAR},
{"guint",      GUINT},
{"guint8",     GUINT8},
{"guint16",    GUINT16},
{"gshort",     GSHORT},
{"if",         IF},
{"include",    INCLUDE}, 
{"int",        INT},
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
%token  <elem> ZERO  
%token  <elem> VectorChan
%token  <elem> Chan
%token  <elem> FloatChan 
%token  <elem> INDENT
%token  <elem> POUND

/* keywords */
%token	BREAK  CASE  CONST  CONTINUE  DEFAULT  DO	
%token 	ELSE  FLOAT FOR  GBOOLEAN  GCHAR  GFLOAT  GINT  GOTO  GUCHAR  GUINT
%token  GUINT8  GUINT16  GSHORT  IF  INCLUDE INT LONG  REAL  REGISTER
%token  RETURN  SIGNED  SIZEOF  STATIC  STRUCT  SWITCH  TYPEDEF
%token  UNION  UNSIGNED  VOID  VOLATILE  WHILE

/* operations */ 
%token	MAX  MIN  ABS  CHAN_CLAMP  WP_CLAMP  
%token  PLUS  MINUS  TIMES  DIVIDE  POWER  LT_PARENTHESIS RT_PARENTHESIS
%token  LT_CURLY  RT_CURLY LT_SQUARE RT_SQUARE 
%token  EQUAL PLUS_EQUAL MINUS_EQUAL TIMES_EQUAL DIVIDE_EQUAL
%token  AND OR EQ NOT_EQ SMALLER GREATER SMALLER_EQ GREATER_EQ NOT ADD SUBTRACT  

%token  COLOR  COLOR_ALPHA  ITERATOR_X  ITERATOR_XY  

%left 		PLUS	MINUS
%left 		TIMES	DIVIDE
%right		EQUAL	PLUS_EQUAL      MINUS_EQUAL     TIMES_EQUAL     DIVIDE_EQUAL 
%right		POWER
%right		EQ
%right		OR  AND 
%nonassoc	NEG

%type   <elem> Expression
%type   <elem> Definition
%type   <elem> Int_List
%type   <elem> Float_List
%type   <elem> Chan_List
%type   <elem> VectorChan_List
%type   <elem> PoundInclDef
%type	<elem> Arguments
%type	<elem> Star 
%type	<elem> Star2  

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
		printf("%selse \n", $1.string); 
		}
	| INDENT FOR LT_PARENTHESIS Expression ';' Expression ';' Expression RT_PARENTHESIS
		{ 
		printf("%sfor (%s; %s; %s)", $1.string, $4.string, $6.string, $8.string); 
		}
	| INDENT IF LT_PARENTHESIS Expression RT_PARENTHESIS
                { 
		printf("%sif (%s)", $1.string, $4.string); 
		}	
	| INDENT ELSE IF LT_PARENTHESIS Expression RT_PARENTHESIS
                { 
		printf("%selse if (%s)", $1.string, $5.string); 
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
	| INDENT Star NAME EQUAL Expression ';'  	
		{
	        char tmp[256];
		elem_t e; 	
		print_name (&e, $3, NOT_DEFINE); 
		do_op_three (&e, e, $5, OP_EQUAL); 
		if (get_sym ($3.string)->type == TYPE_CA_VECTOR ||
		    get_sym ($3.string)->type == TYPE_C_VECTOR) 
		  {
		  sprintf(tmp, "%s%s", $2.string, e.string); 
		  strcpy(e.string, tmp); 
		  }
		sprintf (tmp, "%s%s;", $1.string, e.string);   
		strcpy ($3.string, tmp); 
		print_line ($3); 
		} 
	| INDENT Star NAME PLUS_EQUAL Expression ';'  	
		{ 
		elem_t tmp; 
	        char t[256]; 	
		do_op_three (&tmp, $3, $5, OP_PLUS); 
		print_name (&$3, $3, NOT_DEFINE);
		do_op_three (&$3, $3, tmp, OP_EQUAL); 
		sprintf (t, "%s%s%s;", $1.string, $2.string, $3.string);   
	        strcpy ($3.string, t);	
		print_line ($3); 
		} 
	| INDENT Star NAME MINUS_EQUAL Expression ';'  	
		{ 
		elem_t tmp; 
	        char t[256]; 	
		do_op_three (&tmp, $3, $5, OP_PLUS); 
		print_name (&$3, $3, NOT_DEFINE);
		do_op_three (&$3, $3, tmp, OP_EQUAL); 
		sprintf (t, "%s%s%s;", $1.string, $2.string, $3.string);   
	        strcpy ($3.string, t);	
		print_line ($3); 
		} 
	| INDENT Star NAME TIMES_EQUAL Expression ';'  	
		{ 
		elem_t tmp; 
	        char t[256]; 	
		do_op_three (&tmp, $3, $5, OP_PLUS); 
		print_name (&$3, $3, NOT_DEFINE);
		do_op_three (&$3, $3, tmp, OP_EQUAL); 
		sprintf (t, "%s%s%s;", $1.string, $2.string, $3.string);   
	        strcpy ($3.string, t);	
		print_line ($3); 
		} 
	| INDENT Star NAME DIVIDE_EQUAL Expression ';'  	
		{ 
		elem_t tmp; 
	        char t[256]; 	
		do_op_three (&tmp, $3, $5, OP_PLUS); 
		print_name (&$3, $3, NOT_DEFINE);
		do_op_three (&$3, $3, tmp, OP_EQUAL); 
		sprintf (t, "%s%s%s;", $1.string, $2.string, $3.string);   
	        strcpy ($3.string, t);	
		print_line ($3); 
		} 
	| INDENT Expression ';'   		
		{
	        char tmp[256];	
		sprintf (tmp, "%s%s;", $1.string, $2.string);   
	 	strcpy ($2.string, tmp); 	
		print_line($2); 
		}
	| INDENT ITERATOR_X LT_PARENTHESIS Star2 NAME ',' INT RT_PARENTHESIS ';'
		{
		char tmp[256];
		if (!strcmp($7.string, "1"))
		  {
		  if (get_sym ($5.string)->type == TYPE_CA_VECTOR)
		    {
		    printf ("%sif (%s_a)%s  %s%s_a++", $1.string, $5.string,
			$1.string, $4.string, $5.string);
		    sprintf (tmp, "%s%s%s_c++;", 
		      $1.string, $4.string, $5.string);  
		    }
		  else
		    sprintf (tmp, "%s%s%s_c++;", $1.string, $4.string, $5.string);
		  
		  }
		  else
		    { 
		    if (get_sym ($5.string)->type == TYPE_CA_VECTOR)
		      {
		      printf ("%sif (%s_a)%s  %s%s_a += %s", $1.string, $5.string,
			 $1.string, $4.string, $5.string, $7.string);
		      sprintf (tmp, "%s%s%s_ca += %s;", $1.string, $4.string, $5.string, $7.string); 
		      }
		    else
		      sprintf (tmp, "%s%s%s_c += %s;", $1.string, $4.string, $5.string, $7.string); 
		    }
		strcpy ($5.string, tmp);
		print_line($5); 	
		}
	| INDENT ITERATOR_XY LT_PARENTHESIS Star2 NAME ',' INT ',' INT RT_PARENTHESIS ';' 
		{
	
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
	/*	} 	 
	| Expression EQUAL Expression   
		{ 
		$$=$1; 
		do_op_three (&$$, $1, $3, OP_EQUAL); 
	*/	}
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
		$$=$1; 
		do_op_two (&$$, $1, OP_ADD);
		} 
	| Expression SUBTRACT		
		{ 
		char tmp[256]; 
		$$=$1; 
		sprintf (tmp,"%s--", $1.string);
	        strcpy($$.string, tmp); 
		} 
	| Star NAME				
		{ 
		char tmp[256];
		$$=$2; 
		print_name(&$$, $2, NOT_DEFINE); 
		if (get_sym ($2.string)->type == TYPE_CA_VECTOR ||
		    get_sym ($2.string)->type == TYPE_C_VECTOR) 
		  {
		  sprintf(tmp, "%s%s", $1.string, $$.string); 
		  strcpy($$.string, tmp); 
		  }
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
	| ZERO
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
	| GCHAR Star NAME		
		{ 
		char tmp[256]; 
		$$=$3; 
		sprintf (tmp,"gchar *%s", $3.string); 
	  	strcpy($$.string, tmp); 
		}
       	;
	
Definition:
	  Chan Chan_List		
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
                sprintf (tmp,"%s%s", $1.string, $2.string);
                strcpy($$.string, tmp);
                }
	| GINT Int_List		
		{ 
		char tmp[256]; 
		$$=$2; 
		sprintf (tmp,"gint %s", $2.string);
                strcpy($$.string, tmp); 
		}
	| INT Int_List
		{
           	char tmp[256];
	        $$=$2;
	        sprintf (tmp,"int %s", $2.string);
	        strcpy($$.string, tmp);
	        }
	| FLOAT Float_List
                {
                char tmp[256];
                $$=$2;
                sprintf (tmp,"float %s", $2.string);
                strcpy($$.string, tmp);
                }
	| GUINT Chan_List
		{
           	char tmp[256];
	        $$=$2;
	        sprintf (tmp,"guint %s", $2.string);
	        strcpy($$.string, tmp);
	        }
	| GFLOAT Float_List
                {
                char tmp[256];
                $$=$2;
                sprintf (tmp,"gfloat %s", $2.string);
                strcpy($$.string, tmp);
                }

	| GBOOLEAN Int_List
                {
                char tmp[256];
                $$=$2;
                sprintf (tmp,"int %s", $2.string);
                strcpy($$.string, tmp);
                }
	;


Chan_List:
	  Chan_List ',' Chan_List
		{
		char tmp[256];
		sprintf(tmp, "%s, %s", $1.string, $3.string);
		strcpy($$.string, tmp);
		}
        | Star2 NAME                          
		{
	        char tmp[256];	
		set_dtype($2, TYPE_CHAN); 
		set_type($2, TYPE_SCALER);
	        set_num ($2, 1); 	
	  	$$=$2; 
		print_name (&$$, $2, NOT_DEFINE);
		sprintf(tmp, "%s%s", $1.string, $$.string);
		strcpy($$.string, tmp);
		}
        | Star2 NAME EQUAL FLOAT              
		{ 
		char tmp[256];
		set_dtype($2, TYPE_CHAN);
                set_type($2, TYPE_SCALER);
	        set_num ($2, 1); 	
		$$=$2; 
		print_name (&$$, $2, NOT_DEFINE);
		sprintf(tmp, "%s%s=%s", $1.string, $$.string, $4.string);
		strcpy($$.string, tmp);
		}
        | Star2 NAME EQUAL INT                
		{ 
		char tmp[256];
		set_dtype($2, TYPE_CHAN);
                set_type($2, TYPE_SCALER);
	        set_num ($2, 1); 	
		$$=$2; 
		print_name (&$$, $2, NOT_DEFINE);
		sprintf(tmp, "%s%s=%s", $1.string, $$.string, $4.string);
		strcpy($$.string, tmp);
		}
        | Star2 NAME LT_SQUARE INT RT_SQUARE
		{
		char tmp[256];
		set_dtype($2, TYPE_CHAN); 
		set_type($2, TYPE_VECTOR);
		set_num($2, atoi ($4.string)); 
	        $$=$2;
		print_name (&$$, $2, DEFINE);
		sprintf (tmp, "%s%s", $1.string, $$.string); 
		strcpy ($$.string, tmp); 
		}	
        ;
Int_List:
	  Int_List ',' Int_List
		{
		char tmp[256];
		sprintf(tmp, "%s, %s", $1.string, $3.string);
		strcpy($$.string, tmp);
		}
        | Star2 NAME                          
		{
	        char tmp[256];	
		set_dtype($2, TYPE_INT); 
		set_type($2, TYPE_SCALER);
	        set_num ($2, 1); 	
	  	$$=$2; 
		print_name (&$$, $2, NOT_DEFINE);
		sprintf(tmp, "%s%s", $1.string, $$.string);
		strcpy($$.string, tmp);
		}
        | Star2 NAME EQUAL FLOAT              
		{ 
		char tmp[256];
		set_dtype($2, TYPE_INT);
                set_type($2, TYPE_SCALER);
	        set_num ($2, 1); 	
		$$=$2; 
		print_name (&$$, $2, NOT_DEFINE);
		sprintf(tmp, "%s%s=%s", $1.string, $$.string, $4.string);
		strcpy($$.string, tmp);
		}
        | Star2 NAME EQUAL INT                
		{ 
		char tmp[256];
		set_dtype($2, TYPE_INT);
                set_type($2, TYPE_SCALER);
	        set_num ($2, 1); 	
		$$=$2; 
		print_name (&$$, $2, NOT_DEFINE);
		sprintf(tmp, "%s%s=%s", $1.string, $$.string, $4.string);
		strcpy($$.string, tmp);
		}
        | Star2 NAME LT_SQUARE INT RT_SQUARE
		{
		char tmp[256];
		set_dtype($2, TYPE_INT); 
		set_type($2, TYPE_VECTOR);
		set_num($2, atoi ($4.string)); 
	        $$=$2;
		print_name (&$$, $2, DEFINE);
		sprintf (tmp, "%s%s", $1.string, $$.string); 
		strcpy ($$.string, tmp); 
		}	
        ;
Float_List:
	Float_List ',' Float_List
		{
		char tmp[256];
		sprintf(tmp, "%s, %s", $1.string, $3.string);
		strcpy($$.string, tmp);
		}
        | Star2 NAME                          
		{
	        char tmp[256];	
		set_dtype($2, TYPE_FLOAT);
		set_type($2, TYPE_SCALER);
	        set_num ($2, 1); 	
	  	$$=$2; 
		print_name (&$$, $2, NOT_DEFINE);
		sprintf(tmp, "%s%s", $1.string, $$.string);
		strcpy($$.string, tmp);
		}
        | Star2 NAME EQUAL FLOAT              
		{ 
		char tmp[256];
		set_dtype($2, TYPE_FLOAT);
                set_type($2, TYPE_SCALER);
	        set_num ($2, 1); 	
		$$=$2; 
		print_name (&$$, $2, NOT_DEFINE);
		sprintf(tmp, "%s%s=%s", $1.string, $$.string, $4.string);
		strcpy($$.string, tmp);
		}
        | Star2 NAME EQUAL INT                
		{ 
		char tmp[256];
		set_dtype($2, TYPE_FLOAT);
                set_type($2, TYPE_SCALER);
	        set_num ($2, 1); 	
		$$=$2; 
		print_name (&$$, $2, NOT_DEFINE);
		sprintf(tmp, "%s%s=%s", $1.string, $$.string, $4.string);
		strcpy($$.string, tmp);
		}
        | Star2 NAME LT_SQUARE INT RT_SQUARE
		{
		char tmp[256];
		set_dtype($2, TYPE_FLOAT);
		set_type($2, TYPE_VECTOR);
		set_num($2, atoi ($4.string)); 
	        $$=$2;
		print_name (&$$, $2, DEFINE);
		sprintf (tmp, "%s%s", $1.string, $$.string); 
		strcpy ($$.string, tmp); 
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
        | Star NAME COLOR                         
		{ 
		char tmp[256];
		set_dtype($2, TYPE_CHAN); 
		set_type($2, TYPE_C_VECTOR);
		set_num($2, _NUM_COLOR_CHAN_);
	  	$$=$2; 
		print_name (&$$, $2, DEFINE);
		sprintf(tmp, "%s%s", $1.string, $$.string); 
		print_repeat (&$$, $2, tmp); 
		init_data_varible ($2.string); 
		}
	| Star NAME COLOR_ALPHA
		{
		char tmp[256];
		set_dtype($2, TYPE_CHAN);
		set_type($2, TYPE_CA_VECTOR);
		set_num($2, _NUM_COLOR_CHAN_+1); 
		$$=$2;
		print_name (&$$, $2, DEFINE);
		sprintf(tmp, "%s%s", $1.string, $$.string); 
		print_repeat (&$$, $2, tmp); 
		init_data_varible ($2.string); 
		}
	| Star NAME
		{
		char tmp[256];
		set_dtype($2, TYPE_CHAN);
		set_type($2, TYPE_C_A_VECTOR);
		set_num($2, _NUM_COLOR_CHAN_+1);
		$$=$2;
		print_name (&$$, $2, DEFINE);
		sprintf(tmp, "%s%s", $1.string, $$.string);
		print_repeat (&$$, $2, tmp);
		init_data_varible ($2.string); 
		}
	;

Star:		{ $$.string[0] = '*'; $$.string[1] = '\0'; 	}
	| TIMES Star
		{
		$$ = $2; 
		sprintf ($$.string, "*%s", $2.string);
		}
	| TIMES
		{
		sprintf ($$.string, "*");
		}
		
	; 
	
Star2:	{$$.string[0] = '\0';}
	| TIMES Star
		{
	        $$ = $2;
	        sprintf ($$.string, "*%s", $2.string);
	        }
	| TIMES
	        {
	        sprintf ($$.string, "*");
	        } 
	;
	
%%

#include <stdio.h>

void
rm_varibles (char scope)
{
  int i;
  
  for (i=0; i<cur_nsyms; i++)
    {
    if (symtab[i].scope > scope)
      {
      symtab[i] = symtab[cur_nsyms-1];
      cur_nsyms--; 
      }
    }
  
}

void 
init_image_data (char *indent)
{
  int i;
  elem_t e;
  char tmp[256]; 
 
  /* go through all the symbols find all the color and color alpha vectors */
  for (i=0; i<cur_nsyms; i++)
    {
    if (symtab[i].type == TYPE_CA_VECTOR && !symtab[i].inited) 
      {
      e = symtab[i];
      symtab[i].inited = 1;
      sprintf (tmp, "\n%s%s_ca = %s_data_v;", indent, symtab[i].string, symtab[i].string);
      strcpy (e.string, tmp); 
      print_line (e);  
      }
    if (symtab[i].type == TYPE_C_VECTOR && !symtab[i].inited)
      {
      e = symtab[i];
      symtab[i].inited = 1;
      sprintf (tmp, "%s%s_c = %s_data_v;\n", indent, symtab[i].string, symtab[i].string);
      strcpy (e.string, tmp);
      print_line (e);
      }
    }
  
}
 

void
init_data_varible (char *s)
{
  elem_t *e = get_sym (s); 

  printf ("\n       %s *%s_data[%d];", _Chan_, s, e->num); 
}

void
print_name (elem_t *dest, elem_t src, TYPE_DEF is_define)
{
  char tmp[256],t[256];

  t[0] = '\0';

  if (is_define && get_sym (src.string)->type == TYPE_C_VECTOR)
    {
    sprintf (tmp, "%s_c", get_sym (src.string)->string);
    dest->num = 3;
    }
  else if (is_define && get_sym (src.string)->type == TYPE_CA_VECTOR && 
      !strcmp (src.string, get_sym (src.string)->string))
    {
    sprintf (tmp, "%s_ca", get_sym (src.string)->string);
    dest->num = 4;
    }
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
  dest->type  = src.type;
  if(!is_define)
    dest->num   = src.num;  
  strcpy (dest->string, tmp);
  dest->inited = 0;
}

void
print_repeat (elem_t *dest, elem_t src, char *string)
{

  int i; 
  char tmp[256], t[256];

  t[0]='\0';

  if (get_sym (src.string)->type == TYPE_C_VECTOR)
    {
    for (i=0; i<_NUM_COLOR_CHAN_-1; i++)
      {
      sprintf (tmp, "%s %s_%c,", t, string, _NAME_COLOR_CHAN_[i]);
      strcpy(t, tmp); 
      }
    sprintf (tmp, "%s %s_%c", t, string, _NAME_COLOR_CHAN_[_NUM_COLOR_CHAN_-1]); 
    }
  else if (get_sym (src.string)->type == TYPE_CA_VECTOR)
    {
    for (i=0; i<_NUM_COLOR_CHAN_; i++)
      {
      sprintf (tmp, "%s %s_%c,", t, string, _NAME_COLOR_CHAN_[i]);
      strcpy(t, tmp);
      }
    sprintf (tmp, "%s %s_a", t, string); 
    }
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
	k = src.num; 
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
  sprintf (dest->string, "%s", src.string);
  dest->dtype = src.dtype;
  dest->num = 1;
  dest->inited = 0;
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
    sprintf (tmp, "%s%s%s", _CHAN_CLAMP_PRE_, src.string, _CHAN_CLAMP_SUF_);  
    break; 
  case OP_WP_CLAMP:
    sprintf (tmp, "%s%s%s", _WP_CLAMP_PRE_, src.string, _WP_CLAMP_SUF_);
    break;
  case OP_PARENTHESIS:
    sprintf (tmp, "(%s)", src.string);
    break;
  case OP_ABS:
    sprintf (tmp, "ABS (%s)", src.string);
    break;

  case OP_ADD:
    sprintf (tmp, "%s++", src.string);
    break;
  default:
    break; 
  }

  dest->dtype = src.dtype; 
  dest->type  = src.type;
  dest->num   = src.num;  
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
      sprintf (tmp, "%s + %s", src1.string, src2.string);
      break;
    case OP_MINUS:
      sprintf (tmp, "%s - %s", src1.string, src2.string);
      break;
    case OP_TIMES:
      if (src1.dtype < TYPE_CHAN && src2.dtype < TYPE_CHAN)
        sprintf (tmp, "%s * %s", src1.string, src2.string);
      else if ( src1.dtype == TYPE_CHAN || src2.dtype == TYPE_CHAN)
	{	
      	if (src1.dtype < TYPE_CHAN && src2.dtype < TYPE_CHAN)
	  sprintf (tmp, "%s%s%s%s%s", _TIMES_VV_PRE_, src1.string, _TIMES_VV_MID_, src2.string, _TIMES_VV_SUF_);
	else
	  sprintf (tmp, "%s%s%s%s%s", _TIMES_VS_PRE_, src1.string, _TIMES_VS_MID_, src2.string, _TIMES_VS_SUF_);
	}
      else
	{	
       	if (src1.dtype < TYPE_CHAN && src2.dtype < TYPE_CHAN)
	  sprintf (tmp, "%s * %s * %s", src1.string, src2.string, _WP_NORM_); 
	else
	  sprintf (tmp, "%s * %s", src1.string, src2.string);
	}
      break;
    case OP_DIVIDE:
      if (src1.dtype < TYPE_CHAN && src2.dtype < TYPE_CHAN)
      	sprintf (tmp, "%s / %s", src1.string, src2.string);
      else if ( src1.dtype == TYPE_CHAN || src2.dtype == TYPE_CHAN)
	{
	if (src1.dtype >= TYPE_CHAN &&  src2.dtype >= TYPE_CHAN)
      	  sprintf (tmp, "(%s * %s) / %s", src1.string, _WP_, src2.string);
	else
	  sprintf (tmp, "%s / %s", src1.string, src2.string);
	}
      else
	{	
       	if (src1.dtype >= TYPE_CHAN &&  src2.dtype >= TYPE_CHAN)
	  sprintf (tmp, "(%s * %s) / %s", src1.string, _WP_, src2.string); 
	else
	  sprintf (tmp, "%s / %s", src1.string, src2.string);
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
	case TYPE_INT:
	  switch (src2.dtype)
	    {
	    case TYPE_INT:
	      sprintf (tmp, "%s = %s", src1.string, src2.string);
	      break;
	    case TYPE_FLOAT:
	      sprintf (tmp, "%s = %s", src1.string, src2.string);
	      break;
	    default:
	      yyerror("ERROR: op_EQUAL");
	      exit(1);
	      break;
	    }
	  break;
	case TYPE_FLOAT:
	  switch (src2.dtype)
	    {
	    case TYPE_INT:
	      sprintf (tmp, "%s = %s", src1.string, src2.string);
	      break;
	    case TYPE_FLOAT:
	      sprintf (tmp, "%s = %s", src1.string, src2.string);
	      break;
	    default:
	      yyerror("ERROR: op_EQUAL");
	      exit(1);
	      break;
	   }			     
          break;
	case TYPE_CHAN:
	  switch (src2.dtype)
	    {
	    case TYPE_INT:
	      sprintf (tmp, "%s = %s", src1.string, src2.string);
	      break;
	    case TYPE_FLOAT:
	      sprintf (tmp, "%s = %s%s%s", src1.string, _EQUAL_CFC_PRE_, src2.string, _EQUAL_CFC_SUF_);
	      break;
	    case TYPE_CHAN:
	      sprintf (tmp, "%s = %s", src1.string, src2.string);
	      break;
	    case TYPE_CHANFLOAT:
              sprintf (tmp, "%s = %s%s%s", src1.string, _EQUAL_CFC_PRE_, src2.string, _EQUAL_CFC_SUF_);	      
	      break;
	   }			     
          break;
	case TYPE_CHANFLOAT:
           sprintf (tmp, "%s = %s", src1.string, src2.string);
           break;
	}   
    default:
      break; 
    }
 
  if (src1.dtype == src2.dtype)
   dest->dtype = TYPE_CHAN;
  else if (op == OP_EQUAL)
    dest->dtype = src1.dtype; 
  else 
    dest->dtype = (src1.dtype + src2.dtype) > TYPE_CHANFLOAT? TYPE_CHANFLOAT: (src1.dtype + src2.dtype); 
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
add_sym (char *ss, char scope)
{
  int	i; 
  char *s = strdup(ss);

  i = strlen(s);
  if (i>6)
  if ((s[i-5] == 'a' || s[i-5] == 'c') && s[i-6] == '_')
  {
    s[i-6] = '\0';
    ss[i-4] = '\0';
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
 symtab[cur_nsyms].scope = scope;  
  cur_nsyms++; 
  return symtab[cur_nsyms-1];

}

elem_t* 
get_sym (char *ss)
{

  int   i;
  char *s = strdup(ss);

  i = strlen(s);
  if (i>6)
  if ((s[i-5] == 'a' || s[i-5] == 'c') && s[i-6] == '_')
    {
      s[i-6] = '\0';
      ss[i-4] = '\0';
    }
  
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
     _DATATYPE_		= (char *) strdup ("UINT8");
     _Datatype_		= (char *) strdup ("Uint8");
     _datatype_         = (char *) strdup ("uint8"); 
     _WP_         	= (char *) strdup ("255");
     _WP_NORM_      	= (char *) strdup ("(1.0/255.0)");
     _VectorChan_       = (char *) strdup ("guint8");
     _Chan_         	= (char *) strdup ("guint8");
     _FloatChan_    	= (char *) strdup ("float");
     _MIN_CHAN_     	= (char *) strdup ("0");

     _MAX_CHAN_    	= (char *) strdup ("255");
     _ZERO_CHAN_  	= (char *) strdup ("0");
     _ZERO_	  	= (char *) strdup ("0");

     _CHAN_CLAMP_PRE_  	= (char *) strdup ("CLAMP (");
     _CHAN_CLAMP_SUF_	= (char *) strdup (", 0, 255)");

     _WP_CLAMP_PRE_   	= (char *) strdup ("CLAMP (");
     _WP_CLAMP_SUF_   	= (char *) strdup (", 0, 255)");

     _PLUS_PRE_      	= (char *) strdup ("");
     _PLUS_SUF_      	= (char *) strdup ("");

     _MINUS_PRE_    	= (char *) strdup ("");
     _MINUS_SUF_    	= (char *) strdup ("");

     _TIMES_VS_PRE_  	= (char *) strdup ("");
     _TIMES_VS_MID_	= (char *) strdup ("*");
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

     _DATATYPE_		= (char *) strdup ("UINT16");
     _Datatype_		= (char *) strdup ("Uint16");
     _datatype_         = (char *) strdup ("uint16"); 
     _WP_          	= (char *) strdup ("4095");
     _WP_NORM_     	= (char *) strdup ("(1.0/4095.0)");
     _VectorChan_       = (char *) strdup ("guint16");
     _Chan_         	= (char *) strdup ("guint16");
     _FloatChan_    	= (char *) strdup ("float");
     _MIN_CHAN_      	= (char *) strdup ("0");
     _MAX_CHAN_        	= (char *) strdup ("65535");
     _ZERO_CHAN_      	= (char *) strdup ("0");
     _ZERO_	  	= (char *) strdup ("0");

     _CHAN_CLAMP_PRE_ 	= (char *) strdup ("CLAMP (");
     _CHAN_CLAMP_SUF_  	= (char *) strdup (", 0, 65535)");

     _WP_CLAMP_PRE_   	= (char *) strdup ("CLAMP (");
     _WP_CLAMP_SUF_  	= (char *) strdup (", 0, 4095)");

     _PLUS_PRE_     	= (char *) strdup ("");
     _PLUS_SUF_     	= (char *) strdup ("");

     _MINUS_PRE_     	= (char *) strdup ("");
     _MINUS_SUF_        = (char *) strdup ("");

     _TIMES_VS_PRE_  	= (char *) strdup ("");
     _TIMES_VS_MID_	= (char *) strdup ("*");
     _TIMES_VS_SUF_  	= (char *) strdup ("");

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
     _DATATYPE_		= (char *) strdup ("FLOAT");
     _Datatype_		= (char *) strdup ("Float");
     _datatype_         = (char *) strdup ("float"); 
     _WP_               = (char *) strdup ("1.0");
     _WP_NORM_          = (char *) strdup ("1.0");
     _VectorChan_       = (char *) strdup ("float");
     _Chan_             = (char *) strdup ("float");
     _FloatChan_        = (char *) strdup ("float");
     _MIN_CHAN_         = (char *) strdup ("0");
     _MAX_CHAN_         = (char *) strdup ("1.0");
     _ZERO_CHAN_        = (char *) strdup ("0");
     _ZERO_	  	= (char *) strdup ("0.0");

     _CHAN_CLAMP_PRE_   = (char *) strdup ("");
     _CHAN_CLAMP_SUF_   = (char *) strdup ("");

     _WP_CLAMP_PRE_    	= (char *) strdup ("CLAMP (");
     _WP_CLAMP_SUF_     = (char *) strdup (", 0, 1)");

     _PLUS_PRE_         = (char *) strdup ("");
     _PLUS_SUF_         = (char *) strdup ("");

     _MINUS_PRE_       	= (char *) strdup ("");
     _MINUS_SUF_        = (char *) strdup ("");

     _TIMES_VS_PRE_     = (char *) strdup ("");
     _TIMES_VS_MID_     = (char *) strdup ("*");
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
  char tmp1[20], tmp2[20], tmp3[20];

  _NUM_COLOR_CHAN_ = strlen (color_space); 
  _NAME_COLOR_CHAN_ = (char*) malloc (sizeof(char) * (_NUM_COLOR_CHAN_ + 1));

  for (i=0; i<_NUM_COLOR_CHAN_; i++)
    {
    _NAME_COLOR_CHAN_[i] = color_space[i]; 
    tmp1[i] = toupper (color_space[i]);
    tmp2[i] = tolower (color_space[i]);
    }
  _NAME_COLOR_CHAN_[_NUM_COLOR_CHAN_] = 'a';
  tmp1[_NUM_COLOR_CHAN_] = '\0';
  tmp2[_NUM_COLOR_CHAN_] = '\0';
  _COLORSPACE_ = (char *) strdup (tmp1);
  _colorspace_ = (char *) strdup (tmp2);
  tmp2[0] = tmp1[0];
  _Colorspace_ = (char *) strdup (tmp2); 
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

