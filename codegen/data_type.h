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


EXTERN char *_WP_;   
EXTERN char *_WP_NORM_;       
EXTERN char *_VectorChan_;      
EXTERN char *_CHAN_;      
EXTERN char *_Chan_;      
EXTERN char *_FloatChan_;     	
EXTERN char *_MIN_CHAN_;   
EXTERN char *_MAX_CHAN_;      
EXTERN char *_ZERO_CHAN_; 

EXTERN char *_CHAN_CLAMP_PRE_;		
EXTERN char *_CHAN_CLAMP_SUF_;	

EXTERN char *_WP_CLAMP_PRE_;
EXTERN char *_WP_CLAMP_SUF_;

EXTERN char *_PLUS_PRE_;
EXTERN char *_PLUS_SUF_; 

EXTERN char *_MINUS_PRE_;        
EXTERN char *_MINUS_SUF_;       

EXTERN char *_TIMES_VS_PRE_;	
EXTERN char *_TIMES_VS_SUF_;

EXTERN char *_TIMES_VV_PRE_;        
EXTERN char *_TIMES_VV_MID_;       
EXTERN char *_TIMES_VV_SUF_;       

EXTERN char *_EQUAL_CFC_PRE_;	
EXTERN char *_EQUAL_CFC_SUF_;

/* these are related to the color space */
EXTERN int   _NUM_COLOR_CHAN_;
EXTERN char *_NAME_COLOR_CHAN_;

#undef  EXTERN

void
read_caro_datatypes (char *data_type);


#endif

