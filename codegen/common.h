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
  TYPE_CHAN,
  TYPE_FLOAT
}DATA_TYPE;

typedef enum
{ 
  TYPE_SCALER,
  TYPE_VECTOR, 
  TYPE_C_VECTOR,
  TYPE_CA_VECTOR,  
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
  OP_ADD
}FUNCTION;


/* ELEMENT */
typedef struct
{
  DATA_TYPE 	dtype;
  char          string[256];
  SV_TYPE	type; 
  int		num; 
}elem_t;

typedef	struct node_s node_t; 

struct node_s
{
   DATA_TYPE	dtype;   
   char		string[50];
   node_t	*next;  
};

typedef struct
{
  char 	word[20];	
  int	token;
}keyword_t;

elem_t  add_sym (char *s);
int 	get_keyword (char *s); 
int	yylex (); 

#endif

