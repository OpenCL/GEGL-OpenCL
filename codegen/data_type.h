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

EXTERN char  *WP;   
EXTERN char  *WP_NORM;       
EXTERN char  *DATATYPE;      
EXTERN char  *MIN_CHAN;   
EXTERN char  *MAX_CHAN;      
EXTERN char  *ZERO; 

EXTERN char  *CHAN_CLAMP_PRE;		
EXTERN char  *CHAN_CLAMP_SUF;	

EXTERN char  *WP_CLAMP_PRE;
EXTERN char  *WP_CLAMP_SUF;

EXTERN char  *CHAN_MULT_PRE;        
EXTERN char  *CHAN_MULT_MID;       
EXTERN char  *CHAN_MULT_SUF;       

EXTERN char  *ROUND_PRE;	
EXTERN char  *ROUND_SUF;

/* these are related to the color space */
EXTERN int    NUM_COLOR_CHAN;
EXTERN char  *NAME_COLOR_CHAN[20];

#undef  EXTERN

void
read_caro_datatypes (char *data_type);


#endif

