#ifndef __GIL_TYPES_H__
#define __GIL_TYPES_H__

#include <glib.h>

typedef enum
{
  GIL_TYPE_NONE,
  GIL_INT,
  GIL_FLOAT,
  GIL_TYPE_LAST
} GilType;

typedef union
{
  gint int_value;
  gfloat float_value;
}GilValue;

typedef enum
{
  GIL_BINARY_OP_NONE,
  GIL_PLUS,            /* + */
  GIL_MINUS,           /* - */
  GIL_MULT,            /* * */
  GIL_DIV,             /* / */
  GIL_BINARY_OP_LAST
} GilBinaryOpType;

typedef enum
{
  GIL_UNARY_OP_NONE,
  GIL_UNARY_PLUS,     /* + */
  GIL_UNARY_MINUS,    /* - */
  GIL_UNARY_NEG,      /* ! */
  GIL_UNARY_OP_LAST
} GilUnaryOpType;

#endif /* __GIL_TYPES_H__ */
