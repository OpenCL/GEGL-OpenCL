/*
	FILE:	data_type.h
	DESC:	holds all the macros for all the data types
*/
#ifndef _DATA_TYPE_H_
#define _DATA_TYPE_H_

#include <string.h>

#ifndef EXTERN
#define EXTERN extern
#endif
#ifdef  _PARSER_Y_
#undef  EXTERN
#define EXTERN
#undef  _PARSER_Y_
#endif

EXTERN char  *DATATYPE_STR;      
EXTERN char  *WP_STR;   
EXTERN char  *WP_NORM_STR;       
EXTERN char  *MIN_CHAN_STR;   
EXTERN char  *MAX_CHAN_STR;      
EXTERN char  *ZERO_STR; 

EXTERN char  *CHAN_CLAMP_STR;		

EXTERN char  *WP_CLAMP_STR;

EXTERN char  *CHAN_MULT_STR;        

EXTERN char  *ROUND_STR;	

/* these are related to the color space */
EXTERN int    NUM_COLOR_CHAN;
EXTERN char  *NAME_COLOR_CHAN[20];

#undef  EXTERN

void
read_caro_datatypes (char *data_type);


#endif

