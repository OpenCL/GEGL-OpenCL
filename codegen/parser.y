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
void print (char *string, char *template, char **varible, int num);
void set_dtype (elem_t e, DATA_TYPE dtype);
void set_type (elem_t e, DATA_TYPE dtype);
void set_num (elem_t e, int n);
void read_data_types (char *filename);
void read_channel_names (char *chan_names); 
void init_data_varible (char *s);

int yyerror (char *s); 

#define NSYMS 100           /* maximum number of symbols */
elem_t  symtab[NSYMS];
int     cur_nsyms=0;

  keyword_t keyword_tab[] = {
{"break",      BREAK},
{"boolean",    BOOLEAN}, 
{"case",       CASE},
{"char",       CHAR},
{"const",      CONST},
{"continue",   CONTINUE},
{"default",    DEFAULT},
{"do",         DO},
{"else",       ELSE},
{"float",      FLOAT},
{"for",        FOR},
{"goto",       GOTO},
{"if",         IF},
{"include",    INCLUDE}, 
{"int",        INT},
{"long",       LONG},
{"real",       REAL},
{"register",   REGISTER},
{"return",     RETURN},
{"short",      SHORT},
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
  token_t tok;
}


%token 	<elem> NAME
%token	<elem> FLOAT
%token	<elem> INT
%token  <elem> WP 
%token  <elem> ZERO  
%token  <elem> VectorChan
%token  <elem> Chan
%token  <elem> INDENT
%token  <elem> POUND
%token  <elem> INDENT_CURLY  
%token  <tok>  COMPARE 
%token  <tok>  MIN_MAX
%token  <tok>  ADD_SUB
/* keywords */
%token	BOOLEAN BREAK  CASE  CHAR CONST  CONTINUE  DEFAULT  DO	
%token 	ELSE  FLOAT FOR GOTO 
%token  IF  INCLUDE INT LONG  REAL  REGISTER
%token  RETURN  SHORT SIGNED  SIZEOF  STATIC  STRUCT  SWITCH  TYPEDEF
%token  UNION  UNSIGNED  VOID  VOLATILE  WHILE

/* operations */ 
%token	MAX  MIN  ABS  CHAN_CLAMP  WP_CLAMP  
%token  PLUS  MINUS  TIMES  DIVIDE  POWER  LT_PARENTHESIS RT_PARENTHESIS
%token  LT_CURLY  RT_CURLY LT_SQUARE RT_SQUARE 
%token  EQUAL PLUS_EQUAL MINUS_EQUAL TIMES_EQUAL DIVIDE_EQUAL
%token  AND OR EQ NOT_EQ SMALLER GREATER SMALLER_EQ GREATER_EQ NOT ADD SUBTRACT  

%token  COLOR  COLOR_ALPHA  COLOR_MAYBE_ALPHA  ITERATOR_X  ITERATOR_XY  

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
%type	<elem> PointerVecChan		/* This is for VectorChan, it adds an extra * */ 
%type	<elem> Pointer  

/* tokens for data types */
%token  DT_DATATYPE  DT_WP  DT_WP_NORM  DT_MIN_CHAN DT_MAX_CHAN
%token  DT_ZERO  DT_CHAN_CLAMP  DT_WP_CLAMP  DT_CHAN_MULT  DT_ROUND DT_COMMA 
%token  <tok> DT_NAME
%token  <tok> DT_STRING

%start	Input

%%

Input:	
	
	| Input	Line
	| Input DT_Line 
	;

	
DT_Line:
	;
	| DT_DATATYPE DT_STRING  
		{
		DATATYPE_STR = (char *) strdup (&($2.string[1]));
		DATATYPE_STR[strlen (DATATYPE_STR)-1] = '\0';   
		}
	| DT_WP DT_STRING
		{
		WP_STR = (char *) strdup (&($2.string[1]));
		WP_STR[strlen (WP_STR)-1] = '\0';   
		}
	| DT_WP_NORM DT_STRING
		{
		WP_NORM_STR = (char *) strdup (&($2.string[1]));
		WP_NORM_STR[strlen (WP_NORM_STR)-1] = '\0';   
		}
	| DT_MIN_CHAN DT_STRING
		{
		MIN_CHAN_STR = (char *) strdup (&($2.string[1]));
		MIN_CHAN_STR[strlen (MIN_CHAN_STR)-1] = '\0';   
		}
	| DT_MAX_CHAN DT_STRING
		{
		MAX_CHAN_STR = (char *) strdup (&($2.string[1]));
		MAX_CHAN_STR[strlen (MAX_CHAN_STR)-1] = '\0';   
		}
	| DT_ZERO DT_STRING
		{
		ZERO_STR = (char *) strdup (&($2.string[1]));
		ZERO_STR[strlen (ZERO_STR)-1] = '\0';   
		}
	| DT_CHAN_CLAMP LT_PARENTHESIS DT_NAME RT_PARENTHESIS DT_STRING    
		{
		int i,j=0, len, sublen;
		char tmp[255];
		char sub[255];
		len = strlen ($5.string);
		sublen = strlen ($3.string);

		for(i=1; i<len-sublen-1; i++)
		  {
		  strncpy (sub, &($5.string[i]), sublen); 
		  if (! strcmp ($3.string, sub))
		    {
		    strcpy (&(tmp[j]), "$1");
		    i += sublen-1;
		    j += 2; 
		    }
		  else
		    {
		    tmp[j] = (char) ($5.string[i]); 
		    j++; 
		    }
		  }
		for (; i<len-1; i++)
		  {
		  tmp[j] = (char) ($5.string[i]);
		  j++; 
		  }
		tmp[j] = '\0'; 
		CHAN_CLAMP_STR = (char *) strdup (tmp); 
		printf ("%s", CHAN_CLAMP_STR); 	
		}
	| DT_WP_CLAMP LT_PARENTHESIS DT_NAME RT_PARENTHESIS DT_STRING    
		{
		int i,j=0, len, sublen;
		char tmp[255];
		char sub[255];
		len = strlen ($5.string);
		sublen = strlen ($3.string);

		for(i=1; i<len-sublen-1; i++)
		  {
		  strncpy (sub, &($5.string[i]), sublen); 
		  if (! strcmp ($3.string, sub))
		    {
		    strcpy (&(tmp[j]), "$1");
		    i += sublen-1;
		    j += 2; 
		    }
		  else
		    {
		    tmp[j] = (char) ($5.string[i]); 
		    j++; 
		    }
		  }
		for (; i<len-1; i++)
		  {
		  tmp[j] = (char) ($5.string[i]);
		  j++; 
		  }
		tmp[j] = '\0'; 
		WP_CLAMP_STR = (char *) strdup (tmp); 
		printf ("%s", WP_CLAMP_STR); 	
		}
	| DT_CHAN_MULT LT_PARENTHESIS DT_NAME DT_COMMA DT_NAME RT_PARENTHESIS DT_STRING    
		{
		int i,j=0, len, sublen1, sublen2, flag1, flag2;
		char tmp[255];
		char sub1[255];
		char sub2[255];
		len = strlen ($7.string);
		sublen1 = strlen ($3.string);
		sublen2 = strlen ($5.string);

		for(i=1; i<len-1; i++)
		  {
		  flag1 = flag2 = 0;

		  if (i<len-sublen1)
		    {
		    strncpy (sub1, &($7.string[i]), sublen1);
		    flag1 = 1;
		    }
		  if (i<len-sublen2)
		    {
		    strncpy (sub2, &($7.string[i]), sublen2);
		    flag2 = 1;
		    }

		  if (flag1 && !strcmp ($3.string, sub1))
		    {
		    strcpy (&(tmp[j]), "$1");
		    i += sublen1-1;
		    j += 2;
		    flag2=0;  
		    }
	          else if (flag2 && !strcmp ($5.string, sub2))
		    {
		    strcpy (&(tmp[j]), "$2");
		    i += sublen2-1;
		    j += 2; 
		    }
		  else
		    {
		    tmp[j] = (char) ($7.string[i]); 
		    j++; 
		    }
		  }
		
		tmp[j] = '\0'; 
		CHAN_MULT_STR = (char *) strdup (tmp); 
		printf ("%s", CHAN_MULT_STR); 	
		}
	| DT_ROUND LT_PARENTHESIS DT_NAME RT_PARENTHESIS DT_STRING    
		{
		int i,j=0, len, sublen;
		char tmp[255];
		char sub[255];
		len = strlen ($5.string);
		sublen = strlen ($3.string);

		for(i=1; i<len-sublen-1; i++)
		  {
		  strncpy (sub, &($5.string[i]), sublen); 
		  if (! strcmp ($3.string, sub))
		    {
		    strcpy (&(tmp[j]), "$1");
		    i += sublen-1;
		    j += 2; 
		    }
		  else
		    {
		    tmp[j] = (char) ($5.string[i]); 
		    j++; 
		    }
		  }
		for (; i<len-1; i++)
		  {
		  tmp[j] = (char) ($5.string[i]);
		  j++; 
		  }
		tmp[j] = '\0'; 
		ROUND_STR = (char *) strdup (tmp); 
		printf ("%s", ROUND_STR); 	
		}
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
	| INDENT_CURLY
		{
		printf("%s", $1.string); 
		}
	| VOID				
		{ 
		printf("void "); 
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
	| INDENT CASE Expression ':'  		
		{ 
		printf("%scase %s:", $1.string, $3.string); 
		}
       	| INDENT DEFAULT ':' 	 		
		{ 
		printf("%sdefault:", $1.string); 
		}
	| INDENT ELSE INDENT_CURLY 				
		{ 
		printf("%selse \n%s", $1.string, $3.string); 
		}
	| INDENT FOR LT_PARENTHESIS Expression ';' Expression ';' Expression RT_PARENTHESIS
		{ 
		printf("%sfor (%s; %s; %s)", $1.string, $4.string, $6.string, $8.string); 
		}
	| INDENT IF LT_PARENTHESIS Expression RT_PARENTHESIS INDENT_CURLY
                {
	        char tmp[256];	
		sprintf(tmp, "%sif (%s)%s", $1.string, $4.string, $6.string);
		strcpy ($4.string, tmp);
		print_line ($4); 
		}
	| INDENT ELSE IF LT_PARENTHESIS Expression RT_PARENTHESIS INDENT_CURLY
                {
	        char tmp[256]; 	
		sprintf (tmp, "%selse if (%s)%s", $1.string, $5.string, $7.string); 
		strcpy ($5.string, tmp);
		print_line ($5); 
		}
 	| INDENT RETURN Expression ';'		
		{ 
		printf ("%sreturn (%s);", $1.string, $3.string); 
		}	
	| INDENT SWITCH LT_PARENTHESIS Expression RT_PARENTHESIS
                { 
		char tmp[256];
		sprintf (tmp, "%sswitch (%s)", $1.string, $4.string); 
		strcpy ($4.string, tmp);
		print_line ($4); 
		}
	| INDENT WHILE LT_PARENTHESIS Expression RT_PARENTHESIS INDENT_CURLY
                { 
		char tmp[256];
		sprintf (tmp,"%swhile (%s)%s", $1.string, $4.string, $6.string); 
		strcpy ($4.string, tmp);
		print_line ($4);
		} 	
	| INDENT WHILE LT_PARENTHESIS Expression RT_PARENTHESIS
                { 
		printf("%swhile (%s)%s  {", $1.string, $4.string, $1.string); 
		} 	
	| INDENT Definition ';' 		
		{ 
		printf("%s%s;", $1.string, $2.string); 
		} 
	| INDENT PointerVecChan NAME EQUAL Expression ';'  	
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
	| INDENT PointerVecChan NAME PLUS_EQUAL Expression ';'  	
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
	| INDENT PointerVecChan NAME MINUS_EQUAL Expression ';'  	
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
	| INDENT PointerVecChan NAME TIMES_EQUAL Expression ';'  	
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
	| INDENT PointerVecChan NAME DIVIDE_EQUAL Expression ';'  	
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
	| INDENT ITERATOR_X LT_PARENTHESIS Pointer NAME ',' INT RT_PARENTHESIS ';'
		{
		char tmp[256];
		if (!strcmp($7.string, "1"))
		  {
		  if (get_sym ($5.string)->type == TYPE_C_A_VECTOR)
		    {
		    printf ("%sif (%s_%s)%s  %s%s_%s++;", $1.string, $5.string,
			NAME_COLOR_CHAN[NUM_COLOR_CHAN],
			$1.string, $4.string, $5.string,
			NAME_COLOR_CHAN[NUM_COLOR_CHAN]);
		    sprintf (tmp, "%s%s%s_c++;", 
		      $1.string, $4.string, $5.string);  
		    }
		  else if (get_sym ($5.string)->type == TYPE_CA_VECTOR)
		    {
		    sprintf (tmp, "%s%s%s_c++;", 
		      $1.string, $4.string, $5.string);  
		    }
		  else
		    sprintf (tmp, "%s%s%s_c++;", $1.string, $4.string, $5.string);
		  
		  }
		  else
		    { 
		    if (get_sym ($5.string)->type == TYPE_C_A_VECTOR)
		      {
		      printf ("%sif (%s_%s)%s  %s%s_%s += %s", $1.string, $5.string,
			 NAME_COLOR_CHAN[NUM_COLOR_CHAN], 
			 $1.string, $4.string, $5.string, 
			 NAME_COLOR_CHAN[NUM_COLOR_CHAN],
			 $7.string);
		      sprintf (tmp, "%s%s%s_c += %s;", $1.string, $4.string, $5.string, $7.string); 
		      }
		    else if (get_sym ($5.string)->type == TYPE_CA_VECTOR)
		      {
		      sprintf (tmp, "%s%s%s_ca += %s;", $1.string, $4.string, $5.string, $7.string); 
		      }
		    else
		      sprintf (tmp, "%s%s%s_c += %s;", $1.string, $4.string, $5.string, $7.string); 
		    }
		strcpy ($5.string, tmp);
		print_line($5); 	
		}
	| INDENT ITERATOR_XY LT_PARENTHESIS Pointer NAME ',' INT ',' INT RT_PARENTHESIS ';' 
		{
	
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
	| Expression COMPARE Expression  	
		{ 
		char tmp[256]; 
		$$=$3; 
		sprintf (tmp,"%s %s %s", $1.string, $2.string, $3.string);
                strcpy($$.string, tmp); 
		} 	 
	| MINUS Expression %prec NEG	
		{ 
		$$=$2; 
		do_op_two (&$$, $2, OP_NEG); 
		} 
	| LT_PARENTHESIS Expression RT_PARENTHESIS
		{ 
	        char tmp[256];	
		$$=$2; 
		sprintf (tmp,"(%s)", $2.string);
		strcpy($$.string, tmp);
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
	| ABS LT_PARENTHESIS Expression RT_PARENTHESIS
                {
	        char tmp[256];	
		$$=$3; 
		sprintf (tmp,"ABS (%s)", $3.string);
		strcpy($$.string, tmp);
		} 
	| MIN_MAX LT_PARENTHESIS Expression ',' Expression RT_PARENTHESIS
                { 
		char tmp[256]; 
		$$=$3; 
		sprintf (tmp,"%s (%s,%s)", $1.string, $3.string, $5.string);
	        strcpy($$.string, tmp); 
		}
	| NOT Expression		
		{ 
		char tmp[256]; 
		$$=$2; 
		sprintf (tmp,"!%s", $2.string);
	        strcpy($$.string, tmp); 
		}
	| Expression ADD_SUB		
		{ 
		char tmp[256];
	        $$=$1;
	        sprintf (tmp,"%s%s", $1.string,$2.string);
	        strcpy($$.string, tmp);
		} 
	| PointerVecChan NAME				
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
	| ZERO
		{
		$$=$1;
		print_value(&$$, $1); 
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
	| BOOLEAN Int_List
                {
                char tmp[256];
                $$=$2;
                sprintf (tmp,"boolean %s", $2.string);
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
        | Pointer NAME                          
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
        | Pointer NAME EQUAL FLOAT              
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
        | Pointer NAME EQUAL INT                
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
        | Pointer NAME LT_SQUARE INT RT_SQUARE
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
        | Pointer NAME                          
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
        | Pointer NAME EQUAL FLOAT              
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
        | Pointer NAME EQUAL INT                
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
        | Pointer NAME LT_SQUARE INT RT_SQUARE
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
        | Pointer NAME                          
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
        | Pointer NAME EQUAL FLOAT              
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
        | Pointer NAME EQUAL INT                
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
        | Pointer NAME LT_SQUARE INT RT_SQUARE
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
        | PointerVecChan NAME COLOR                         
		{ 
		char tmp[256];
		set_dtype($2, TYPE_CHAN); 
		set_type($2, TYPE_C_VECTOR);
		set_num($2, NUM_COLOR_CHAN);
	  	$$=$2; 
		print_name (&$$, $2, DEFINE);
		sprintf(tmp, "%s%s", $1.string, $$.string); 
		print_repeat (&$$, $2, tmp); 
		init_data_varible ($2.string); 
		}
	| PointerVecChan NAME COLOR_ALPHA
		{
		char tmp[256];
		set_dtype($2, TYPE_CHAN);
		set_type($2, TYPE_CA_VECTOR);
		set_num($2, NUM_COLOR_CHAN+1); 
		$$=$2;
		print_name (&$$, $2, DEFINE);
		sprintf(tmp, "%s%s", $1.string, $$.string); 
		print_repeat (&$$, $2, tmp); 
		init_data_varible ($2.string); 
		}
	| PointerVecChan NAME COLOR_MAYBE_ALPHA
		{
		char tmp[256];
		set_dtype($2, TYPE_CHAN);
		set_type($2, TYPE_C_A_VECTOR);
		set_num($2, NUM_COLOR_CHAN+1);
		$$=$2;
		print_name (&$$, $2, DEFINE);
		sprintf(tmp, "%s%s", $1.string, $$.string);
		print_repeat (&$$, $2, tmp);
		init_data_varible ($2.string); 
		}
	;

PointerVecChan:		
		{ $$.string[0] = '*'; $$.string[1] = '\0'; 	}
	| TIMES PointerVecChan
		{
		$$ = $2; 
		sprintf ($$.string, "*%s", $2.string);
		}
	| TIMES
		{
		sprintf ($$.string, "*");
		}
		
	; 
	
Pointer:	{$$.string[0] = '\0';}
	| TIMES Pointer 
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
      sprintf (tmp, "%s%s_ca = %s_data_v;", indent, symtab[i].string, symtab[i].string);
      strcpy (e.string, tmp); 
      print_line (e);  
      }
    if (symtab[i].type == TYPE_C_VECTOR && !symtab[i].inited)
      {
      e = symtab[i];
      symtab[i].inited = 1;
      sprintf (tmp, "%s%s_c = %s_data_v;", indent, symtab[i].string, symtab[i].string);
      strcpy (e.string, tmp);
      print_line (e);
      }
    }
  
}
 

void
init_data_varible (char *s)
{
  int i; 
  char tmp[20]; 
  elem_t *e = get_sym (s); 
 
  for (i=0; i<SCOPE; i++)
    {
    tmp[i*2  ] = ' ';
    tmp[i*2+1] = ' ';
    }
  tmp[i*2  ] = '\0'; 
  printf ("\n%s%s *%s_data[%d];", tmp, DATATYPE_STR, s, e->num); 
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
    for (i=0; i<NUM_COLOR_CHAN-1; i++)
      {
      sprintf (tmp, "%s %s_%s,", t, string, NAME_COLOR_CHAN[i]);
      strcpy(t, tmp); 
      }
    sprintf (tmp, "%s %s_%s", t, string, NAME_COLOR_CHAN[NUM_COLOR_CHAN-1]); 
    }
  else if (get_sym (src.string)->type == TYPE_CA_VECTOR)
    {
    for (i=0; i<NUM_COLOR_CHAN; i++)
      {
      sprintf (tmp, "%s %s_%s,", t, string, NAME_COLOR_CHAN[i]);
      strcpy(t, tmp);
      }
    sprintf (tmp, "%s %s_%s", t, string, NAME_COLOR_CHAN[NUM_COLOR_CHAN]); 
    }
  strcpy (dest->string, tmp);
  
}
void
print_line (elem_t src)
{

  int i,j,k=1;
  int l = strlen (src.string);
 
  if (src.type>1 || (src.type && src.num == NUM_COLOR_CHAN)) /* if it is a vector */ 
  for (i=0; i<k; i++)
    {
    for(j=0; j<l; j++)
      {
      if (j < l-2 && src.string[j] == '_' && src.string[j+1] == 'c' && src.string[j+2] == 'a')
	{
	printf("_%s", NAME_COLOR_CHAN[i]);
	k = NUM_COLOR_CHAN + 1;
	j += 2;
	}
      else if (j < l-1 && src.string[j] == '_' && src.string[j+1] == 'c')
	{
	printf("_%s", NAME_COLOR_CHAN[i]);
	k = NUM_COLOR_CHAN; 	
	j++;
	}
      else if (j < l-1 && src.string[j] == '_' && src.string[j+1] == 'v')
	{
	printf("[%d]", i);
	k = src.num; 
	j++;
	}
      else if (j < l-1 && src.string[j] == '_' && src.string[j+1] == 'a')
	{
	printf("_%s", NAME_COLOR_CHAN[NUM_COLOR_CHAN]);
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
	else if (j < l-1 && src.string[j] == '_' && src.string[j+1] == 'a')
	  {
	  printf("_%s", NAME_COLOR_CHAN[NUM_COLOR_CHAN]);
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
print (char *string, char *template, char *varible[], int num)
{
 
  int i, j=0, k, len, *len_varible;
  len = strlen (template);

  len_varible = (int*) malloc (sizeof(int)*num);
  
  for (i=0; i<num; i++)
    {
    len_varible[i] = strlen (varible[i]); 
    }
  
  for (i=0; i<len-1; i++)
    {
    if (template[i] == '$' && isdigit ((int) template[i+1]))
      {
      k = (int) atoi (&(template[i+1]))-1; 
      strcpy (&(string[j]), varible[k]);
      j += len_varible[k];
      i ++; 
      }
    else 
      {
      string[j] = template[i];
      j ++; 
      }
    }
  string[j] = template[i];  
  string[j+1] = '\0';  
}
void
do_op_two (elem_t *dest, elem_t src, FUNCTION op)
{
  char tmp[256];
  char *t[1]; 
  switch (op)
  {
  case OP_NEG:
    sprintf (tmp, "-%s", src.string);
    break;
  case OP_CHAN_CLAMP:
    t[0] = src.string; 
    print (tmp, CHAN_CLAMP_STR, t, 1); 
    break; 
  case OP_WP_CLAMP:
    t[0] = src.string; 
    print (tmp, WP_CLAMP_STR, t, 1); 
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
  char *t[2]; 

  /* error checking */
  if (src1.num != src2.num  && src1.num != 1 && src2.num != 1)
    {
    yyerror("ERROR: you are trying to preform an operation on varibles
	that dont have the same number of channels");
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
      	if (src1.dtype >= TYPE_CHAN && src2.dtype >= TYPE_CHAN)
	  {	
      	  t[0] = src1.string;
	  t[1] = src2.string;
	  print (tmp, CHAN_MULT_STR, t, 2); 
	  }
	else
	  sprintf (tmp, "%s * %s", src1.string, src2.string);
	}
      else
	{	
       	if (src1.dtype >= TYPE_CHAN && src2.dtype >= TYPE_CHAN)
	  sprintf (tmp, "%s * %s * %s", src1.string, src2.string, WP_NORM_STR); 
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
      	  sprintf (tmp, "(%s * %s) / %s", src1.string, WP_STR, src2.string);
	else
	  sprintf (tmp, "%s / %s", src1.string, src2.string);
	}
      else
	{	
       	if (src1.dtype >= TYPE_CHAN &&  src2.dtype >= TYPE_CHAN)
	  sprintf (tmp, "(%s * %s) / %s", src1.string, WP_STR, src2.string); 
	else
	  sprintf (tmp, "%s / %s", src1.string, src2.string);
	}
      break;
    case OP_EQUAL:
  	
      /* error checking */
      if (src1.num != src2.num && !(src1.num > src2.num && src2.num == 1))
	{
	yyerror("ERROR: You trying to assign a varible to another varible
	    and they have different number of channels");
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
	      yyerror("ERROR: programming error");
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
	      t[0] = src2.string;
	      print (tmp, CHAN_MULT_STR, t, 1); 
	      strcpy (dest->string, tmp);
	      sprintf (tmp, "%s = %s", src1.string, dest->string);
	      break;
	    case TYPE_CHAN:
	      sprintf (tmp, "%s = %s", src1.string, src2.string);
	      break;
	    case TYPE_CHANFLOAT:
	      t[0] = src2.string;
	      print (tmp, CHAN_MULT_STR, t, 1); 
	      strcpy (dest->string, tmp);
	      sprintf (tmp, "%s = %s", src1.string, dest->string);
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
    yyerror("Error: the sysmble was not defined");
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
    yyerror("Error: the symbol was not define");
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
    yyerror("Error: the symbol was not define");
    exit(1);      /* cannot continue */
    }
}


/* look up a symbol table entry, add if not present */
elem_t
add_sym (char *ss, char scope)
{
  int	i; 
  char *s = strdup(ss);

  for (i=0; i<cur_nsyms; i++) 
  {
    /* is it already here? */
    if(!strcmp(symtab[i].string, s)){

	yyerror("Error: Varible already declared");
	exit(1);
	
    }
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

  for (i=0; i<cur_nsyms; i++)
  {
    /* is it already here? */
    if(!strcmp(symtab[i].string, s))
            return &(symtab[i]);
  }

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

  yyerror("Error: you have not defined the varible");
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
read_data_types (char *filename)
{

}

void
read_channel_names (char *chan_names)
{

  int i=0;
  char *tmp; 

  if (!(tmp = (char*) strtok (chan_names, ",")))
    {
    exit (0);
    }

  NAME_COLOR_CHAN[i] = tmp;

  while ((tmp = (char*) strtok (NULL, ",")))
    {
    i++;
    NAME_COLOR_CHAN[i] = tmp; 
    }
  
  NUM_COLOR_CHAN = i;

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
  int i=1;
  yydebug = 1; 
  if (argc != 6)
    {
    printf ("ERROR: need to specify a file and channel names\n");
    return -1; 
    }
 
  while (i<argc)
    {
    if (!strcmp (argv[i], "--channel-data-file"))
      {
      i++;
      open_file (argv[i]);
      yyparse ();
      close_file ();  
      }
    if (!strcmp (argv[i], "--channel-names"))
      {
      i++;
      read_channel_names (argv[i]); 
      }
    i++; 
    } 
  open_file (argv[5]);  
  yyparse();
  close_file (); 

  return 0; 
}

