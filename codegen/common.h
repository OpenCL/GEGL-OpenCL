/*
	FILE:	common.h
	DESC:	defines structs, enum 	
*/

#ifndef _COMMON_H_
#define _COMMON_H_

#include "data_type.h"

typedef enum
{
  DEFINE,
  NOT_DEFINE
}TYPE_DEF;

/* DATA TYPES */
typedef enum
{
  TYPE_INT,
  TYPE_FLOAT,
  TYPE_CHAN,
  TYPE_CHANFLOAT
}DATA_TYPE;

typedef enum
{ 
  TYPE_SCALER,
  TYPE_VECTOR, 
  TYPE_C_VECTOR,
  TYPE_CA_VECTOR,  
  TYPE_C_A_VECTOR,  
}SV_TYPE;

/* FUNCTIONS */
typedef enum
{
  OP_PLUS,
  OP_MINUS,
  OP_TIMES,
  OP_DIVIDE,
  OP_NEG, 
  OP_MAX,
  OP_MIN,
  OP_ABS, 
  OP_WP_CLAMP,
  OP_CHAN_CLAMP, 
  OP_EQUAL,
  OP_PARENTHESIS,
  OP_AND,
  OP_OR,    
}FUNCTION;


/* ELEMENT */
typedef struct
{
  DATA_TYPE 	dtype;
  char          string[256];
  SV_TYPE	type; 
  int		num; 
  char		inited; 
  char          scope; 
}elem_t;

typedef struct
{
  char 	word[20];	
  int	token;
}keyword_t;

#ifdef  _LEXER_C_
int SCOPE = 0;
#undef  _LEXER_C_
#else
extern int SCOPE;
#endif


elem_t  add_sym (char *s, char scope);
elem_t* get_sym (char *sym); 
void	init_image_data (char *indent); 
int 	get_keyword (char *s); 
int	yylex (); 
void    rm_varibles (char scope); 


#endif

