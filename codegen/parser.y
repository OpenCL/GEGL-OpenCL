%{
#ifndef _PARSER_Y_
#define _PARSER_Y_
#endif

#include "common.h"
#include <string.h> 
#include <stdlib.h>
   
    
void do_op_two (elem_t *dest, elem_t src, FUNCTION op);
void do_op_three (elem_t *dest, elem_t src1, elem_t src2, FUNCTION op);
void print_value (elem_t *dest, elem_t src);
void print_name (elem_t *dest, elem_t src, TYPE_DECL is_define);
void print_pixel_variable (elem_t *dest, elem_t src, char *string); 
void print_line (elem_t src);
void print_with_template (char *string, char *template, char **varible, int num);
void set_dtype (elem_t e, DATA_TYPE dtype);
void set_svtype (elem_t e, SV_TYPE svtype);
void set_num (elem_t e, int n);
void read_channel_names (char *chan_names); 
void add_pixel_variables (char *s);

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

  dt_keyword_t dt_keyword_tab[] = {
	{"DATATYPE", 0, DT_DATATYPE},
	{"PROMOTE_TYPE", 0, DT_PROMOTE_TYPE},
	{"SIGNED_PROMOTE_TYPE", 0, DT_SIGNED_PROMOTE_TYPE},
	{"WP", 0, DT_WP},
	{"WP_NORM", 0, DT_WP_NORM},
	{"CHANNEL_MIN", 0, DT_CHANNEL_MIN},
	{"CHANNEL_MAX", 0, DT_CHANNEL_MAX},
	{"ZERO", 0, DT_ZERO},
	{"CHANNEL_CLAMP", 1, DT_CHANNEL_CLAMP},
	{"WP_CLAMP", 1, DT_WP_CLAMP},
	{"CHANNEL_MULT", 1, DT_CHANNEL_MULT},
	{"CHANNEL_ROUND", 1, DT_CHANNEL_ROUND}, 
	{"PRINT", 1, DT_PRINT}, 
	{"EXTERNAL_INIT", 1, DT_EXTERNAL_INIT},
	{"", 0, -1}
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
%token  <elem> Pixel
%token  <elem> Channel
%token  <elem> INDENT
%token  <elem> POUND
%token  <elem> INDENT_CURLY  
%token  <elem> VARIABLE  
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
%token	MAX  MIN  ABS  CHANNEL_CLAMP  WP_CLAMP  PRINT    
%token  PLUS  MINUS  TIMES  DIVIDE  POWER  LT_PARENTHESIS RT_PARENTHESIS
%token  LT_CURLY  RT_CURLY LT_SQUARE RT_SQUARE 
%token  EQUAL PLUS_EQUAL MINUS_EQUAL TIMES_EQUAL DIVIDE_EQUAL
%token  AND OR EQ NOT_EQ SMALLER GREATER SMALLER_EQ GREATER_EQ NOT ADD SUBTRACT  

%token  COLOR  COLOR_ALPHA  COLOR_MAYBE_ALPHA  ITERATOR_X  ITERATOR_Y ITERATOR_XY 
%token  PROMOTE_TYPE SIGNED_PROMOTE_TYPE 

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
%type   <elem> Channel_List
%type   <elem> Pixel_List

/* tokens for data types */
%token  DT_DATATYPE  DT_PROMOTE_TYPE  DT_SIGNED_PROMOTE_TYPE  DT_WP  DT_WP_NORM  
%token  DT_CHANNEL_MIN DT_CHANNEL_MAX  DT_PRINT  DT_EXTERNAL_INIT  
%token  DT_ZERO  DT_CHANNEL_CLAMP  DT_WP_CLAMP  DT_CHANNEL_MULT  DT_CHANNEL_ROUND DT_COMMA 
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
		int i=0;
		while ($2.string[i] == '\t' || $2.string[i] == ' ')
		  i++; 
		DATATYPE_STR = (char *) strdup (&($2.string[i]));
		}
	| DT_PROMOTE_TYPE DT_STRING  
		{
		int i=0;
		while ($2.string[i] == '\t' || $2.string[i] == ' ')
		  i++; 
		PROMOTE_TYPE_STR = (char *) strdup (&($2.string[i]));
		}
	| DT_SIGNED_PROMOTE_TYPE DT_STRING  
		{
		int i=0;
		while ($2.string[i] == '\t' || $2.string[i] == ' ')
		  i++; 
		SIGNED_PROMOTE_TYPE_STR = (char *) strdup (&($2.string[i]));
		}
	| DT_WP DT_STRING
		{
		int i=0;
		while ($2.string[i] == '\t' || $2.string[i] == ' ')
		  i++; 
		WP_STR = (char *) strdup (&($2.string[i]));
		}
	| DT_WP_NORM DT_STRING
		{
		int i=0;
		while ($2.string[i] == '\t' || $2.string[i] == ' ')
		  i++; 
		WP_NORM_STR = (char *) strdup (&($2.string[i]));
		}
	| DT_CHANNEL_MIN DT_STRING
		{
		int i=0;
		while ($2.string[i] == '\t' || $2.string[i] == ' ')
		  i++; 
		CHANNEL_MIN_STR = (char *) strdup (&($2.string[i]));
		}
	| DT_CHANNEL_MAX DT_STRING
		{
		int i=0;
		while ($2.string[i] == '\t' || $2.string[i] == ' ')
		  i++; 
		CHANNEL_MAX_STR = (char *) strdup (&($2.string[i]));
		}
	| DT_ZERO DT_STRING
		{
		int i=0;
		while ($2.string[i] == '\t' || $2.string[i] == ' ')
		  i++; 
		ZERO_STR = (char *) strdup (&($2.string[i]));
		}
	| DT_CHANNEL_CLAMP LT_PARENTHESIS DT_NAME RT_PARENTHESIS DT_STRING    
		{
		int i=0, j=0, len, sublen;
		char tmp[255];
		char sub[255];
		len = strlen ($5.string);
		sublen = strlen ($3.string);

		while ($5.string[i] == '\t' || $5.string[i] == ' ')
		  i++;
		
		for(i=i; i<len; i++)
		  {
		  loop1:
		 
		  if (i<=len-2)
		    {
		      if ($5.string[i] == '\\' && $5.string[i+1] == '\n')
			{
			  i += 2;
		    
			  while ($5.string[i] == '\t' || $5.string[i] == ' ')
			    i++;
			  goto loop1;
			} 
		    } 
		  
		  if (i<=len-sublen)
		    {
		      strncpy (sub, &($5.string[i]), sublen);
		      sub[sublen] = '\0'; 
		      if (!strcmp ($3.string, sub))
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
		  else
		    {
		      tmp[j] = (char) ($5.string[i]); 
		      j++; 
		    }
		  }
		tmp[j] = '\0'; 
		CHANNEL_CLAMP_STR = (char *) strdup (tmp); 
		}
	| DT_WP_CLAMP LT_PARENTHESIS DT_NAME RT_PARENTHESIS DT_STRING    
		{
		int i=0, j=0, len, sublen;
		char tmp[255];
		char sub[255];
		len = strlen ($5.string);
		sublen = strlen ($3.string);

		while ($5.string[i] == '\t' || $5.string[i] == ' ')
		  i++;
		
		for(i=i; i<len; i++)
		  {
		  
		  loop2:
		 
		  if (i<=len-2)
		    {
		      if ($5.string[i] == '\\' && $5.string[i+1] == '\n')
			{
			  i += 2;
		    
			  while ($5.string[i] == '\t' || $5.string[i] == ' ')
			    i++;
			  goto loop2;
			} 
		    } 

		  if (i<=len-sublen)
		    { 
		      strncpy (sub, &($5.string[i]), sublen); 
		      sub[sublen] = '\0'; 
		      if (!strcmp ($3.string, sub))
			{
			  strcpy (&(tmp[j]), "$1");
			  i += sublen;
			  j += 2; 
			}
		      else
			{
			  tmp[j] = (char) ($5.string[i]);
			  j++;
			}
		    }
		  else
		    {
		      tmp[j] = (char) ($5.string[i]); 
		      j++; 
		    }
		  }
		tmp[j] = '\0'; 
		WP_CLAMP_STR = (char *) strdup (tmp); 
		}
	| DT_CHANNEL_MULT LT_PARENTHESIS DT_NAME DT_COMMA DT_NAME RT_PARENTHESIS DT_STRING    
		{
		int i=0,j=0, len, sublen1, sublen2, flag1, flag2;
		char tmp[255];
		char sub1[255];
		char sub2[255];
		len = strlen ($7.string);
		sublen1 = strlen ($3.string);
		sublen2 = strlen ($5.string);

		while ($7.string[i] == '\t' || $7.string[i] == ' ')
		  i++;
		
		for(i=i; i<len; i++)
		  {
		  flag1 = flag2 = 0;

		  loop3:
		 
		  if (i<=len-2)
		    {
		      if ($7.string[i] == '\\' && $7.string[i+1] == '\n')
			{
			  i += 2;
		    
			  while ($7.string[i] == '\t' || $7.string[i] == ' ')
			    i++;
			  goto loop3;
			} 
		    } 

		  if (i<=len-sublen1)
		    {
		    strncpy (sub1, &($7.string[i]), sublen1);
		    flag1 = 1;
		    }
		  if (i<=len-sublen2)
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
		CHANNEL_MULT_STR = (char *) strdup (tmp); 
		}
	| DT_CHANNEL_ROUND LT_PARENTHESIS DT_NAME RT_PARENTHESIS DT_STRING    
		{
		int i=0, j=0, len, sublen;
		char tmp[255];
		char sub[255];
		len = strlen ($5.string);
		sublen = strlen ($3.string);

		while ($5.string[i] == '\t' || $5.string[i] == ' ')
		  i++;

		for(i=i; i<len; i++)
		  {
		  loop4:
		 
		  if (i<=len-2)
		    {
		      if ($5.string[i] == '\\' && $5.string[i+1] == '\n')
			{
			  i += 2;
		    
			  while ($5.string[i] == '\t' || $5.string[i] == ' ')
			    i++;
			  goto loop4;
			} 
		    } 

		  if (i<=len-sublen)
		    {
		      strncpy (sub, &($5.string[i]), sublen); 
		      sub[sublen] = '\0'; 
		      if (!strcmp ($3.string, sub))
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
		  else
		    {
		      tmp[j] = (char) ($5.string[i]);
		      j++;
		    }

		  }
		if (tmp[j-1] == '\\') tmp[j-1] = '\0'; 
		tmp[j] = '\0'; 
		CHANNEL_ROUND_STR = (char *) strdup (tmp); 
		}
	| DT_PRINT LT_PARENTHESIS DT_NAME RT_PARENTHESIS DT_STRING    
		{
		int i=0, j=0, len, sublen;
		char tmp[255];
		char sub[255];
		len = strlen ($5.string);
		sublen = strlen ($3.string);

		while ($5.string[i] == '\t' || $5.string[i] == ' ')
		  i++;
		
		for(i=i; i<len; i++)
		  {
		  loop5:
		 
		  if (i<=len-2)
		    {
		      if ($5.string[i] == '\\' && $5.string[i+1] == '\n')
			{
			  i += 2;
		    
			  while ($5.string[i] == '\t' || $5.string[i] == ' ')
			    i++;
			  goto loop5;
			} 
		    } 

		  if (i<=len-sublen)
		    {
		      strncpy (sub, &($5.string[i]), sublen); 
		      sub[sublen] = '\0'; 
		      if (!strcmp ($3.string, sub))
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
		  else
		    {
		      tmp[j] = (char) ($5.string[i]);
		      j++;
		    }

		  }
		if (tmp[j-1] == '\\') tmp[j-1] = '\0'; 
		tmp[j] = '\0'; 
		PRINT_STR = (char *) strdup (tmp); 
		}
	| DT_EXTERNAL_INIT LT_PARENTHESIS DT_NAME RT_PARENTHESIS DT_STRING   
		{
		int i=0;
		while ($5.string[i] != '.')
		  i++; 
		EXTERNAL_INIT_STR = (char *) strdup (&($5.string[i+1]));
		EXTERNAL_INIT_STR[strlen (EXTERNAL_INIT_STR)] = '\0';   
		}
	;

	
Line:
	; 
	| "\n" 				
		{ 
		  printf("==>\n"); 
		}
	| INDENT 
		{
		  printf("%s ", $1.string); 
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
	| INDENT CASE NAME ':'  		
		{ 
	 	  printf("%scase %s:", $1.string, $3.string); 
		}
       	| INDENT DEFAULT ':' 	 		
		{ 
		  printf("%sdefault:", $1.string); 
		}
	| INDENT ELSE INDENT_CURLY 				
		{ 
		  printf("%selse%s", $1.string, $3.string); 
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
	| INDENT NAME EQUAL Expression ';'  	
		{
		  char tmp[256];
		  int i; 
		  elem_t e; 	
		  print_name (&e, $2, NOT_DECL); 
		  do_op_three (&e, e, $4, OP_EQUAL); 
		  if (get_sym ($2.string)->svtype == TYPE_CA_VECTOR ||
		      get_sym ($2.string)->svtype == TYPE_C_VECTOR ||
		      get_sym ($2.string)->svtype == TYPE_C_A_VECTOR) 
		    {
		      sprintf(tmp, "*%s", e.string); 
		      strcpy(e.string, tmp); 
		    }
		  sprintf (tmp, "%s%s;", $1.string, e.string);   
		  strcpy (e.string, tmp); 
		  print_line (e);

		  if (get_sym ($2.string)->svtype == TYPE_C_A_VECTOR && 
		      !strcmp (get_sym ($2.string)->string, $2.string))
		    {
		      i = strlen (tmp);
		      while (i != 1)
			{
			  i--;
			  if (tmp[i] == 'c' && tmp[i-1] == '$')
			    tmp[i] = 'a'; 
			}
		      sprintf (e.string, "%sif (%s_has_%s)\n  %s", $1.string, 
			  $2.string, NAME_COLOR_CHANNEL[NUM_COLOR_CHANNEL], &tmp[1]); 
		      e.num = 1;
		      print_line (e);
		    }

		} 
	| INDENT Expression ';'   		
		{
		  char tmp[256];	
		  sprintf (tmp, "%s%s;", $1.string, $2.string);   
		  strcpy ($2.string, tmp); 	
		  print_line($2); 
		}
	| INDENT ITERATOR_X LT_PARENTHESIS NAME ',' INT RT_PARENTHESIS ';'
		{
		  char tmp[256];
		  if (!strcmp($6.string, "1"))
		    {
		      if (get_sym ($4.string)->svtype == TYPE_C_A_VECTOR)
			{
			  char t[20];
			  strcpy (t, $4.string);  
			  sprintf (tmp, "%s%s$c++;", $1.string, $4.string);  
			  strcpy ($4.string, tmp);
			  print_line($4);
			  printf ("%sif (%s_has_%s)%s  %s_%s++;", $1.string, t,
			      NAME_COLOR_CHANNEL[NUM_COLOR_CHANNEL],
			      $1.string, t,
			      NAME_COLOR_CHANNEL[NUM_COLOR_CHANNEL]);
			  printf ("\n"); 
			}
		      else if (get_sym ($4.string)->svtype == TYPE_CA_VECTOR)
			{
			  sprintf (tmp, "%s%s$ca++;", $1.string, $4.string);  
			  strcpy ($4.string, tmp);
			  print_line($4); 
			  printf ("\n"); 
			}
		      else
			{
			  sprintf (tmp, "%s%s$c++;", $1.string, $4.string);
			  strcpy ($4.string, tmp);
			  print_line($4); 
			  printf ("\n");  
			}
		    }
		  else
		    { 
		      if (get_sym ($4.string)->svtype == TYPE_C_A_VECTOR)
			{
			  char t[20];
			  strcpy (t, $4.string);  
			  sprintf (tmp, "%s%s%s$c += %s;", $1.string, $4.string, $6.string); 
			  strcpy ($4.string, tmp);
			  print_line($4); 
			  printf ("%sif (%s_has_%s)%s  %s_%s += %s;", $1.string, t,
			      NAME_COLOR_CHANNEL[NUM_COLOR_CHANNEL], 
			      $1.string, t,  
			      NAME_COLOR_CHANNEL[NUM_COLOR_CHANNEL],
			      $6.string);
			  printf ("\n"); 
			}
		      else if (get_sym ($4.string)->svtype == TYPE_CA_VECTOR)
			{
			  sprintf (tmp, "%s%s$ca += %s;", $1.string, $4.string, $6.string); 
			  strcpy ($4.string, tmp);
			  print_line($4); 
			  printf ("\n"); 
			}
		      else
			{
			  sprintf (tmp, "%s%s$c += %s;", $1.string, $4.string, $6.string); 
			  strcpy ($4.string, tmp);
			  print_line($4); 
			  printf ("\n"); 
			} 
		    }
		}
	| INDENT ITERATOR_X LT_PARENTHESIS NAME ',' NAME RT_PARENTHESIS ';'
		{
		  char tmp[256];
		  if (get_sym ($4.string)->svtype == TYPE_C_A_VECTOR)
		    {
		      char t[20];
		      strcpy (t, $4.string);  
		      sprintf (tmp, "%s%s$c += %s;", $1.string, $4.string, $6.string); 
		      strcpy ($4.string, tmp);
		      print_line($4); 
		      printf ("%sif (%s_has_%s)%s  %s_%s += %s;", $1.string, t,
			  NAME_COLOR_CHANNEL[NUM_COLOR_CHANNEL], 
			  $1.string, t,  
			  NAME_COLOR_CHANNEL[NUM_COLOR_CHANNEL],
			  $6.string);
		      printf ("\n"); 
		    }
		  else if (get_sym ($4.string)->svtype == TYPE_CA_VECTOR)
		    {
		      sprintf (tmp, "%s%s$ca += %s;", $1.string, $4.string, $6.string); 
		      strcpy ($4.string, tmp);
		      print_line($4); 
		      printf ("\n"); 
		    }
		  else
		    {
		      sprintf (tmp, "%s%s$c += %s;", $1.string, $4.string, $6.string); 
		      strcpy ($4.string, tmp);
		      print_line($4); 
		      printf ("\n"); 
		    } 
		}
	| INDENT ITERATOR_Y LT_PARENTHESIS NAME ',' INT RT_PARENTHESIS ';'
		{
		char tmp[256];
		if (get_sym ($4.string)->svtype == TYPE_C_A_VECTOR)
		  {
		    char t[20];
		    strcpy (t, $4.string);  
		    sprintf (tmp, "%s%s$c += %s * %s_rowstride$v;", $1.string, $4.string, 
			$6.string, $4.string); 
		    strcpy ($4.string, tmp);
		    print_line($4); 
		    printf ("%sif (%s_has_%s)%s  %s_%s += %s * %s_rowstride[%d];", 
			$1.string, t,
			NAME_COLOR_CHANNEL[NUM_COLOR_CHANNEL], 
			$1.string, t,  
			NAME_COLOR_CHANNEL[NUM_COLOR_CHANNEL],
			$6.string, t, NUM_COLOR_CHANNEL);
		    printf ("\n"); 
		  }
		else if (get_sym ($4.string)->svtype == TYPE_CA_VECTOR)
		  {
		    sprintf (tmp, "%s%s$ca += %s * %s_rowstride$v;", $1.string, $4.string, 
			$6.string, $4.string); 
		    strcpy ($4.string, tmp);
		    print_line($4); 
		    printf ("\n"); 
		  }
		else
		  {
		    sprintf (tmp, "%s%s$c += %s * %s_rowstride$v;", $1.string, $4.string, 
			$6.string, $4.string); 
		    strcpy ($4.string, tmp);
		    print_line($4); 
		    printf ("\n"); 
		  } 
		}
	| INDENT ITERATOR_XY LT_PARENTHESIS NAME ',' INT ',' INT RT_PARENTHESIS ';' 
		{
	
		}
	| INDENT PRINT LT_PARENTHESIS NAME RT_PARENTHESIS ';'
		{
		  char tmp[256];
		  char t2[256];
		  char *t[1];
		  strcpy (t2, $4.string);
		  print_name (&$4, $4, NOT_DECL);
		  if ($4.svtype > 1)
		    {
		      sprintf (tmp, "*%s", $4.string);
		      strcpy ($4.string, tmp);
		    }
		  t[0] = $4.string;
		  print_with_template (tmp, PRINT_STR, t, 1);
		  sprintf ($4.string, "%s%s;", $1.string, tmp);
		  print_line ($4);
		  if (get_sym (t2)->svtype == TYPE_C_A_VECTOR)
		    {
		      printf ("%sif (%s_has_%s)%s  ", $1.string, t2,
			  NAME_COLOR_CHANNEL[NUM_COLOR_CHANNEL],
			  $1.string);
		      if ($4.svtype > 1)
			sprintf (tmp, "*%s$a", t2);
		      else
			sprintf (tmp, "%s$a", t2);
		      t[0] = tmp;
		      print_with_template (t2, PRINT_STR, t, 1);
		      sprintf ($4.string, "%s;", t2);
		      $4.num = 1;
		      print_line ($4);
		    }
		}	  
	;

	
Expression:
	  PROMOTE_TYPE Expression
	  	{
		$$=$2;
      		sprintf ($$.string, "%s %s", PROMOTE_TYPE_STR, $2.string); 		
		}
        | SIGNED_PROMOTE_TYPE Expression
	  	{
		$$=$2;
      		sprintf ($$.string, "%s %s", SIGNED_PROMOTE_TYPE_STR, $2.string); 		
		}
	| Expression PLUS Expression	
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
	        if ($3.num)	
		  $$=$3; 
		else
		  $$=$1;
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
	| CHANNEL_CLAMP LT_PARENTHESIS Expression RT_PARENTHESIS
                { 
		$$=$3; 
		do_op_two (&$$, $3, OP_CHANNEL_CLAMP); 
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
	| NAME				
		{ 
		char tmp[256];
		$$=$1; 
		print_name(&$$, $1, NOT_DECL); 
		if (get_sym ($1.string)->svtype == TYPE_CA_VECTOR ||
		    get_sym ($1.string)->svtype == TYPE_C_VECTOR ||
		    get_sym ($1.string)->svtype == TYPE_C_A_VECTOR) 
		  {
		  sprintf(tmp, "*%s", $$.string); 
		  strcpy($$.string, tmp); 
		  }
		}
	| FLOAT				
		{ 
		$1.num = 1;
		$$=$1; 
		print_value(&$$, $1); 
		}
	| INT				
		{
		$1.num = 1;
		$$=$1; 
		print_value(&$$, $1); 
		} 
	| WP				
		{ 
		$1.num = 0;
		$$=$1; 
		print_value(&$$, $1); 
		}
	| ZERO
		{
		$1.num = 0;
		$$=$1;
		print_value(&$$, $1); 
		}
	| VARIABLE  
		{
		$$ = $1;
		} 	  
	;

	
Definition:
	  Channel Channel_List		
		{ 
		char tmp[256]; 
		$$=$2; 
		sprintf (tmp,"%s %s", $1.string, $2.string);
                strcpy($$.string, tmp); 
		}
	| Pixel Pixel_List
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


Channel_List:
	  Channel_List ',' Channel_List
		{
		char tmp[256];
		sprintf(tmp, "%s, %s", $1.string, $3.string);
		strcpy($$.string, tmp);
		}
        | NAME                          
		{
		set_dtype($1, TYPE_CHANNEL); 
		set_svtype($1, TYPE_SCALER);
	        set_num ($1, 1); 	
	  	$$=$1; 
		print_name (&$$, $1, DECL);
		}
        | NAME EQUAL FLOAT              
		{ 
		char tmp[256];
		set_dtype($1, TYPE_CHANNEL);
                set_svtype($1, TYPE_SCALER);
	        set_num ($1, 1); 	
		$$=$1; 
		print_name (&$$, $1, DECL);
		sprintf(tmp, "%s=%s", $$.string, $3.string);
		strcpy($$.string, tmp);
		}
        | NAME EQUAL INT                
		{ 
		char tmp[256];
		set_dtype($1, TYPE_CHANNEL);
                set_svtype($1, TYPE_SCALER);
	        set_num ($1, 1); 	
		$$=$1; 
		print_name (&$$, $1, DECL);
		sprintf(tmp, "%s=%s", $$.string, $3.string);
		strcpy($$.string, tmp);
		}
        | NAME LT_SQUARE INT RT_SQUARE
		{
		char tmp[256];
		set_dtype($1, TYPE_CHANNEL); 
		set_svtype($1, TYPE_VECTOR);
		set_num($1, atoi ($3.string)); 
	        $$=$1;
		print_name (&$$, $1, DECL);
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
        | NAME                          
		{
		set_dtype($1, TYPE_INT); 
		set_svtype($1, TYPE_SCALER);
	        set_num ($1, 1); 	
	  	$$=$1; 
		print_name (&$$, $1, DECL);
		}
        | NAME EQUAL FLOAT              
		{ 
		char tmp[256];
		set_dtype($1, TYPE_INT);
                set_svtype($1, TYPE_SCALER);
	        set_num ($1, 1); 	
		$$=$1; 
		print_name (&$$, $1, DECL);
		sprintf(tmp, "%s=%s", $$.string, $3.string);
		strcpy($$.string, tmp);
		}
        | NAME EQUAL INT                
		{ 
		char tmp[256];
		set_dtype($1, TYPE_INT);
                set_svtype($1, TYPE_SCALER);
	        set_num ($1, 1); 	
		$$=$1; 
		print_name (&$$, $1, DECL);
		sprintf(tmp, "%s=%s", $$.string, $3.string);
		strcpy($$.string, tmp);
		}
        | NAME LT_SQUARE INT RT_SQUARE
		{
		char tmp[256];
		set_dtype($1, TYPE_INT); 
		set_svtype($1, TYPE_VECTOR);
		set_num($1, atoi ($3.string)); 
	        $$=$1;
		print_name (&$$, $1, DECL);
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
        | NAME                          
		{
		set_dtype($1, TYPE_FLOAT);
		set_svtype($1, TYPE_SCALER);
	        set_num ($1, 1); 	
	  	$$=$1; 
		print_name (&$$, $1, DECL);
		}
        | NAME EQUAL FLOAT              
		{ 
		char tmp[256];
		set_dtype($1, TYPE_FLOAT);
                set_svtype($1, TYPE_SCALER);
	        set_num ($1, 1); 	
		$$=$1; 
		print_name (&$$, $1, DECL);
		sprintf(tmp, "%s=%s", $$.string, $3.string);
		strcpy($$.string, tmp);
		}
        | NAME EQUAL INT                
		{ 
		char tmp[256];
		set_dtype($1, TYPE_FLOAT);
                set_svtype($1, TYPE_SCALER);
	        set_num ($1, 1); 	
		$$=$1; 
		print_name (&$$, $1, DECL);
		sprintf(tmp, "%s=%s", $$.string, $3.string);
		strcpy($$.string, tmp);
		}
        | NAME LT_SQUARE INT RT_SQUARE
		{
		char tmp[256];
		set_dtype($1, TYPE_FLOAT);
		set_svtype($1, TYPE_VECTOR);
		set_num($1, atoi ($3.string)); 
	        $$=$1;
		print_name (&$$, $1, DECL);
		strcpy ($$.string, tmp); 
		}	
        ;

Pixel_List:
          Pixel_List ',' Pixel_List            
		{ 
		char tmp[256]; 
		$$=$3;
                sprintf (tmp, "%s, %s", $1.string, $3.string);
                strcpy($$.string, tmp); 
		}
        | NAME COLOR                         
		{ 
		char tmp[256];
		set_dtype($1, TYPE_CHANNEL); 
		set_svtype($1, TYPE_C_VECTOR);
		set_num($1, NUM_COLOR_CHANNEL);
	  	$$=$1; 
		print_name (&$$, $1, DECL);
		sprintf(tmp, "*%s", $$.string); 
		print_pixel_variable (&$$, $1, tmp); 
		add_pixel_variables ($1.string); 
		}
	| NAME COLOR_ALPHA
		{
		char tmp[256];
		set_dtype($1, TYPE_CHANNEL);
		set_svtype($1, TYPE_CA_VECTOR);
		set_num($1, NUM_COLOR_CHANNEL+1); 
		$$=$1;
		print_name (&$$, $1, DECL);
		sprintf(tmp, "*%s", $$.string); 
		print_pixel_variable (&$$, $1, tmp); 
		add_pixel_variables ($1.string); 
		}
	| NAME COLOR_MAYBE_ALPHA
		{
		char tmp[256];
		set_dtype($1, TYPE_CHANNEL);
		set_svtype($1, TYPE_C_A_VECTOR);
		set_num($1, NUM_COLOR_CHANNEL+1);
		$$=$1;
		print_name (&$$, $1, DECL);
		sprintf(tmp, "*%s", $$.string);
		print_pixel_variable (&$$, $1, tmp);
		add_pixel_variables ($1.string); 
		}
	;


%%

#include <stdio.h>

void
rm_sym_from_symtab (char scope)
{
  int i;

  for (i=0; i<cur_nsyms; i++)
    {
      if (symtab[i].scope > scope)
	{
	  symtab[i] = symtab[cur_nsyms-1];
	  cur_nsyms--; 
	  i--; 
	}
    }
}

void 
init_image_data (char *indent)
{
  int i;
  elem_t e;
  char tmp[256]; 

  /* go through all the symbols find all the pixel variables */
  for (i=0; i<cur_nsyms; i++)
    {
      if (!symtab[i].inited)
	switch (symtab[i].svtype)
	  {
	  case TYPE_CA_VECTOR:
	    e = symtab[i];
	    symtab[i].inited = 1;
	    sprintf (tmp, "%s%s$ca = %s_data$v;", indent, symtab[i].string, symtab[i].string);
	    strcpy (e.string, tmp); 
	    print_line (e);  
	    printf ("\n"); 
	    break;
	  case TYPE_C_A_VECTOR:
	    e = symtab[i];
	    symtab[i].inited = 1;
	    sprintf (tmp, "%s%s$c = %s_data$v;", indent, symtab[i].string, symtab[i].string);
	    strcpy (e.string, tmp); 
	    print_line (e); 
	    sprintf (tmp, "%sif (%s_has$a)%s  %s$a = %s_data[%d];", 
		indent, symtab[i].string, indent, symtab[i].string, symtab[i].string, NUM_COLOR_CHANNEL);
	    strcpy (e.string, tmp);
	    print_line (e); 
	    printf ("\n"); 
	    break;
	  case TYPE_C_VECTOR: 
	    e = symtab[i];
	    symtab[i].inited = 1;
	    sprintf (tmp, "%s%s$c = %s_data$v;", indent, symtab[i].string, symtab[i].string);
	    strcpy (e.string, tmp);
	    print_line (e);
	    printf ("\n"); 
	    break;
	  default:
	    break; 
	  }
    }
}


void
add_pixel_variables (char *s)
{
  int i; 
  char tmp[20], name[20]; 
  elem_t *e = get_sym (s); 

  for (i=0; i<SCOPE; i++)
    {
      tmp[i*2  ] = ' ';
      tmp[i*2+1] = ' ';
    }
  tmp[i*2  ] = '\0';

  printf ("\n%s%s *%s_data[%d];", tmp, DATATYPE_STR, s, e->num);
  if (e->svtype == TYPE_C_A_VECTOR)
    {  
      elem_t e; 
      sprintf (name, "%s_has_%s", s, NAME_COLOR_CHANNEL[NUM_COLOR_CHANNEL]);
      e = add_sym (name, SCOPE);
      set_dtype (e, TYPE_INT);
      set_svtype (e, TYPE_SCALER);
      set_num (e, 1);  
      printf ("\n%sgboolean %s;", tmp, name); 

    }
    /*{
     elem_t e;
     sprintf (name, "%s_pixelstride", s);
     e = add_sym (name, SCOPE);
     set_dtype (e, TYPE_INT);
     set_svtype (e, TYPE_SCALER);
     set_num (e, 1);
     printf ("\n%sgint %s;", tmp, name); 
    }*/ 
  /*printf ("\n%s%s *%s_rowstride[%d];", tmp, DATATYPE_STR, s, e->num);*/
}

void
print_name (elem_t *dest, elem_t src, TYPE_DECL is_decl)
{
  char tmp[256],t[256];
  t[0] = '\0';
  dest->num = 1; 

  if (is_decl == NOT_DECL)
    {
      if (get_sym (src.string)->svtype == TYPE_C_VECTOR ||
	  (get_sym (src.string)->svtype == TYPE_C_A_VECTOR &&
	  !strcmp (src.string, get_sym (src.string)->string))) 
	{
	  dest->num = NUM_COLOR_CHANNEL;
	  sprintf (tmp, "%s$c", get_sym (src.string)->string);
	}
      else if (get_sym (src.string)->svtype == TYPE_CA_VECTOR &&
	       !strcmp (src.string, get_sym (src.string)->string)) 
	{
	  dest->num = NUM_COLOR_CHANNEL + 1;
	  sprintf (tmp, "%s$ca", get_sym (src.string)->string);
	}
      else if (get_sym (src.string)->svtype == TYPE_CA_VECTOR ||
	       get_sym (src.string)->svtype == TYPE_C_A_VECTOR)
	{
	  int l = strlen (src.string);
	  if (src.string[l-1] == 'a' && 
	      src.string[l-2] == '$')
	    {
	      dest->num = 1;
	      sprintf (tmp, "%s", src.string);
	    }
	  if (src.string[l-1] == 'c' && 
	      src.string[l-2] == '$')
	    {
	      dest->num = NUM_COLOR_CHANNEL; 
	      sprintf (tmp, "%s", src.string);
	    }
	}
      else if (get_sym (src.string)->svtype == TYPE_VECTOR)
	{
	  dest->num = get_sym (src.string)->num; 
	  sprintf (tmp, "%s$v", src.string);
	}
      else
	{
	  dest->num = 1; 
	  sprintf (tmp, "%s", src.string);
	}
    }
  else 
    {
      if (get_sym (src.string)->svtype == TYPE_VECTOR)
	sprintf (tmp, "%s[%d]", src.string, get_sym (src.string)->num);
      else
	sprintf (tmp, "%s", src.string);
    }

  if(is_decl == DECL)
    {
      dest->inited = 0;
      dest->num   = src.num;
    }  
  dest->dtype = src.dtype;
  dest->svtype  = src.svtype;
  strcpy (dest->string, tmp);
}

void
print_pixel_variable (elem_t *dest, elem_t src, char *string)
{

  int i; 
  char tmp[256], t[256];

  t[0]='\0';

  if (get_sym (src.string)->svtype == TYPE_C_VECTOR)
    {
      for (i=0; i<NUM_COLOR_CHANNEL-1; i++)
	{
	  sprintf (tmp, "%s %s_%s,", t, string, NAME_COLOR_CHANNEL[i]);
	  strcpy(t, tmp); 
	}
      sprintf (tmp, "%s %s_%s", t, string, NAME_COLOR_CHANNEL[NUM_COLOR_CHANNEL-1]); 
    }
  else if (get_sym (src.string)->svtype == TYPE_CA_VECTOR)
    {
      for (i=0; i<NUM_COLOR_CHANNEL; i++)
	{
	  sprintf (tmp, "%s %s_%s,", t, string, NAME_COLOR_CHANNEL[i]);
	  strcpy(t, tmp);
	}
      sprintf (tmp, "%s %s_%s", t, string, NAME_COLOR_CHANNEL[NUM_COLOR_CHANNEL]); 
    }
  else
    {
      for (i=0; i<NUM_COLOR_CHANNEL; i++)
	{
	  sprintf (tmp, "%s %s_%s,", t, string, NAME_COLOR_CHANNEL[i]);
	  strcpy(t, tmp);
	}
      sprintf (tmp, "%s %s_%s=NULL", t, string, NAME_COLOR_CHANNEL[NUM_COLOR_CHANNEL]);
    }
  strcpy (dest->string, tmp);

}
void
print_line (elem_t src)
{

  int i,j,k=1;
  int is_alpha; 
  int l = strlen (src.string);

  if (src.svtype>1 || (src.svtype && src.num == NUM_COLOR_CHANNEL)) /* if it is a vector */ 
    {
      for (i=0; i<k; i++)
	{
	  is_alpha = 0; 
	  for(j=0; j<l; j++)
	    {
	      if (j < l-1 && src.string[j] == '$' && src.string[j+1] == 'e')
		{
		  src.string[j+1] = 'v'; 
		}
      	      if (j < l-2 && src.string[j] == '$' && src.string[j+1] == 'c' && src.string[j+2] == 'a')
		{
		  
		  printf("_%s", NAME_COLOR_CHANNEL[i]);
		  k = NUM_COLOR_CHANNEL + 1;
		  j += 2;
		
		}
	      else if (j < l-1 && src.string[j] == '$' && src.string[j+1] == 'c')
		{
		  printf("_%s", NAME_COLOR_CHANNEL[i]);
		  k = NUM_COLOR_CHANNEL; 	
		  j++;
		}
	      else if (j < l-1 && src.string[j] == '$' && src.string[j+1] == 'v')
		{
		  if (src.svtype == TYPE_C_A_VECTOR && is_alpha == 1)
		    printf("[%d]", NUM_COLOR_CHANNEL);
		  else 
		    printf("[%d]", i);

		  if (src.svtype != TYPE_C_A_VECTOR)
		    k = src.num; 
		  j++;
		}
	      else if (j < l-1 && src.string[j] == '$' && src.string[j+1] == 'a')
		{
		  is_alpha = 1; 
		  printf("_%s", NAME_COLOR_CHANNEL[NUM_COLOR_CHANNEL]);
		  j++; 	
		}
	      else
		{
		  printf("%c", src.string[j]);
		}
	    }
	}
    }
  else if (src.svtype)
    for (i=0; i<src.num; i++)
      {
	for(j=0; j<l; j++)
	  {
	    if (j < l-1 && src.string[j] == '$' && src.string[j+1] == 'e')
		{
		  j += 2; 	  
		}
	    if (j < l-1 && src.string[j] == '$' && src.string[j+1] == 'v')
	      {
		printf("[%d]", i);
		j++;
	      }
	    else if (j < l-1 && src.string[j] == '$' && src.string[j+1] == 'a')
	      {
		printf("_%s", NAME_COLOR_CHANNEL[NUM_COLOR_CHANNEL]);
		j++;
	      }
	    else
	      {
		printf("%c", src.string[j]); 
	      }
	  }
      }
  else if (src.dtype == TYPE_CHANNEL || src.dtype == TYPE_FLOAT) 
    {
      for (i=0; i<src.num; i++)
	{
	  for(j=0; j<l; j++)
	    {
	      if (j < l-1 && src.string[j] == '$' && src.string[j+1] == 'e')
		{
		  j += 2; 	  
		}
	      if (j < l-1 && src.string[j] == '$' && src.string[j+1] == 'v')
		{
		  printf("[%d]", i);
		  j++;
		}
	      else if (j < l-1 && src.string[j] == '$' && src.string[j+1] == 'a')
		{
		  printf("_%s", NAME_COLOR_CHANNEL[NUM_COLOR_CHANNEL]);
		  j++;
		}
	      else
		{
		  printf("%c", src.string[j]); 	
		}
	    } 
	}
    }
  else
    printf("%s", src.string);

}

void
print_value (elem_t *dest, elem_t src)
{
  sprintf (dest->string, "%s", src.string);
  dest->dtype = src.dtype;
  dest->num = src.num;
  dest->inited = 0;
}

void
print_with_template (char *string, char *template, char *varible[], int num)
{

  int i, j=0, k, len, *len_varible;
  len = strlen (template);

  len_varible = (int*) malloc (sizeof (int) * num);

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
    case OP_CHANNEL_CLAMP:
      t[0] = src.string; 
      print_with_template (tmp, CHANNEL_CLAMP_STR, t, 1); 
      break; 
    case OP_WP_CLAMP:
      t[0] = src.string; 
      print_with_template (tmp, WP_CLAMP_STR, t, 1); 
      break;
    default:
      break; 
    }

  dest->dtype = src.dtype; 
  dest->svtype  = src.svtype;
  dest->num   = src.num;  
  strcpy (dest->string, tmp);

}

void
do_op_three (elem_t *dest, elem_t src1, elem_t src2, FUNCTION op)
{
  char tmp[256];
  char *t[9]; 

  /* error checking */
  if (src1.num != src2.num && src1.num && src2.num && src1.num != 1 && src2.num != 1)
    {
      yyerror("ERROR: you are trying to preform an operation on vector variables
	  that dont have the same number of channels");
	exit(1); 
    }

  switch (op)
    {
    case OP_PLUS:
      if (src1.num != src2.num && src1.num && src2.num)
	{
	  yyerror("ERROR: you are trying to preform an operation on vector variables
	      that dont have the same number of channels");
	    exit(1);
	}
      sprintf (tmp, "%s + %s", src1.string, src2.string);
      break;
    case OP_MINUS:
      if (src1.num != src2.num && src1.num && src2.num)
	{
	  yyerror("ERROR: you are trying to preform an operation on vector variables
	      that dont have the same number of channels");
	    exit(1);
	}
      sprintf (tmp, "%s - %s", src1.string, src2.string);
      break;
    case OP_TIMES:
      if (src1.dtype < TYPE_CHANNEL && src2.dtype < TYPE_CHANNEL)
	sprintf (tmp, "%s * %s", src1.string, src2.string);
      else if ( src1.dtype == TYPE_CHANNEL || src2.dtype == TYPE_CHANNEL)
	{	
	  if (src1.dtype >= TYPE_CHANNEL && src2.dtype >= TYPE_CHANNEL)
	    {	
	      t[0] = src1.string;
	      t[1] = src2.string;
	      print_with_template (tmp, CHANNEL_MULT_STR, t, 2); 
	    }
	  else
	    sprintf (tmp, "%s * %s", src1.string, src2.string);
	}
      else
	{	
	  if (src1.dtype >= TYPE_CHANNEL && src2.dtype >= TYPE_CHANNEL)
	    sprintf (tmp, "%s * %s * %s", src1.string, src2.string, WP_NORM_STR); 
	  else
	    sprintf (tmp, "%s * %s", src1.string, src2.string);
	}
      break;
    case OP_DIVIDE:
      if (src1.dtype < TYPE_CHANNEL && src2.dtype < TYPE_CHANNEL)
	sprintf (tmp, "%s / %s", src1.string, src2.string);
      else if ( src1.dtype == TYPE_CHANNEL || src2.dtype == TYPE_CHANNEL)
	{
	  if (src1.dtype >= TYPE_CHANNEL &&  src2.dtype >= TYPE_CHANNEL)
	    sprintf (tmp, "(%s * %s) / %s", src1.string, WP_STR, src2.string);
	  else
	    sprintf (tmp, "%s / %s", src1.string, src2.string);
	}
      else
	{	
	  if (src1.dtype >= TYPE_CHANNEL &&  src2.dtype >= TYPE_CHANNEL)
	    sprintf (tmp, "(%s * %s) / %s", src1.string, WP_STR, src2.string); 
	  else
	    sprintf (tmp, "%s / %s", src1.string, src2.string);
	}
      break;
    case OP_EQUAL:

      /* error checking */
      if (src1.num != src2.num && !(src1.num > src2.num && src2.num < 1))
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
	    case TYPE_CHANNEL:
	      sprintf (tmp, "%s = %s", src1.string, src2.string);
	      break;
	    case TYPE_CHANNELFLOAT:
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
	    case TYPE_CHANNEL:
	      sprintf (tmp, "%s = %s", src1.string, src2.string);
	      break;
	    case TYPE_CHANNELFLOAT:
	      sprintf (tmp, "%s = %s", src1.string, src2.string);
	      break; 
	    default:
	      yyerror("ERROR: programming error");
	      exit(1);
	      break;
	    }			     
	  break;
	case TYPE_CHANNEL:
	  switch (src2.dtype)
	    {
	    case TYPE_INT:
	      sprintf (tmp, "%s = %s", src1.string, src2.string);
	      break;
	    case TYPE_FLOAT:
	      t[0] = src2.string;
	      print_with_template (tmp, CHANNEL_ROUND_STR, t, 1); 
	      strcpy (dest->string, tmp);
	      sprintf (tmp, "%s = %s", src1.string, dest->string);
	      break;
	    case TYPE_CHANNEL:
	      sprintf (tmp, "%s = %s", src1.string, src2.string);
	      break;
	    case TYPE_CHANNELFLOAT:
	      t[0] = src2.string;
	      print_with_template (tmp, CHANNEL_ROUND_STR, t, 1); 
	      strcpy (dest->string, tmp);
	      sprintf (tmp, "%s = %s", src1.string, dest->string);
	      break;
	    }			     
	  break;
	case TYPE_CHANNELFLOAT:
	  sprintf (tmp, "%s = %s", src1.string, src2.string);
	  break;
	}   
    default:
      break; 
    }

  if (src1.dtype == src2.dtype)
    dest->dtype = TYPE_CHANNEL;
  else if (op == OP_EQUAL)
    dest->dtype = src1.dtype; 
  else 
    dest->dtype = (src1.dtype + src2.dtype) > TYPE_CHANNELFLOAT? TYPE_CHANNELFLOAT: 
      (src1.dtype + src2.dtype); 
  dest->svtype  = (src1.svtype > src2.svtype)?src1.svtype:src2.svtype;
  dest->num   = (src1.num > src2.num)?src1.num:src2.num;
  strcpy (dest->string, tmp);
}


void
set_dtype (elem_t e, DATA_TYPE dtype)
{
  int   i;

  for (i=0; i<cur_nsyms; i++)
    {
      /* is it already here? */
      if(!strcmp(symtab[i].string, e.string))
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
set_svtype (elem_t e, SV_TYPE svtype)
{
  int   i;

  for (i=0; i<cur_nsyms; i++)
    {
      /* is it already here? */
      if(!strcmp(symtab[i].string, e.string))
	{
	  symtab[i].svtype = svtype;
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
add_sym (char *s, char scope)
{
  int	i; 

  for (i=0; i<cur_nsyms; i++) 
    {
      /* is it already here? */
      if(!strcmp (symtab[i].string, s)){
	    printf ("\n%s\n", s); 
	    yyerror ("Error: Varible already declared");
	    exit(1);

      }
    }

  if (cur_nsyms == NSYMS) 
    {
      yyerror ("Too many symbols");
      exit(1);      /* cannot continue */
    }

  strcpy (symtab[cur_nsyms].string, s);
  symtab[cur_nsyms].scope = scope;  
  symtab[cur_nsyms].inited = 0; 
  cur_nsyms++; 
  return symtab[cur_nsyms-1];

}

elem_t* 
get_sym (char *s)
{

  int   i;
  char c[255];


  for (i=0; i<cur_nsyms; i++)
    {
      /* is it already here? */
      if(!strcmp(symtab[i].string, s))
	return &(symtab[i]);
    }

  strcpy (c, s);

  i = strlen(s);
  if (i>6)
    if (!strcmp ("_color", &(s[i-6])) || !strcmp ("_alpha", &(s[i-6])))
      {
	c[i-6] = '\0';
	s[i-4] = '\0';
	s[i-6] = '$'; 
	goto find;
      }

  if (i>2)
    if ((s[i-1] == 'a' || s[i-1] == 'c') && s[i-2] == '$')
      {
	c[i-2] = '\0';
      } 

find:
  for (i=0; i<cur_nsyms; i++)
    {
      /* is it already here? */
      if(!strcmp(symtab[i].string, c))
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

dt_keyword_t*
get_dt_keyword (char *keywd)
{

  int i=0; 

  while (dt_keyword_tab[i].arg != 9)
    {
      if (!strcmp (dt_keyword_tab[i].word, keywd))
	return &(dt_keyword_tab[i]);
      else
	i ++;
    }

  return NULL; 
}


void
read_channel_names (char *chan_names)
{

  int i=0;
  char *tmp; 

  if (!(tmp = (char*) strtok (chan_names, ",")))
    {
      yyerror("Error: reading channel names");
      exit (0);
    }

  NAME_COLOR_CHANNEL[i] = tmp;

  while ((tmp = (char*) strtok (NULL, ",")))
    {
      i++;
      NAME_COLOR_CHANNEL[i] = tmp; 
    }

  NUM_COLOR_CHANNEL = i+1;
  NAME_COLOR_CHANNEL[NUM_COLOR_CHANNEL] = (char*) strdup ("alpha"); 
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
  /* uncomment this to see lexer and parser debug info */
  /* yydebug = 1;*/    
  
  if (argc < 5)
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
      if (!strcmp (argv[i], "--pixel-data-file"))
	{
	  i++;
	  strcpy (gegl_pixel, argv[i]); 
	}
      i++; 
    }

  if (argc == 6) 
    open_file (argv[5]);  

  else 
    open_file ("stdin");

  yyparse();
  close_file (); 

  return 0; 
}

