#ifndef __GEGL_TYPES_H__
#define __GEGL_TYPES_H__

#include <gtk/gtk.h>

typedef enum
{
  COLORSPACE_NONE,
  GRAY,
  RGB
} GeglColorSpace;

typedef enum
{
  COMPOSITE_REPLACE,
  COMPOSITE_OVER,
  COMPOSITE_IN,
  COMPOSITE_OUT,
  COMPOSITE_ATOP,
  COMPOSITE_XOR
}GeglCompositeMode;

typedef enum
{
  COLOR_WHITE,
  COLOR_BLACK,
  COLOR_RED,
  COLOR_GREEN,
  COLOR_BLUE,
  COLOR_GRAY,
  COLOR_HALF_WHITE,
  COLOR_TRANSPARENT,
  COLOR_WHITE_TRANSPARENT,
  COLOR_BLACK_TRANSPARENT
} GeglColorConstant;

typedef enum
{
  DATATYPE_NONE,
  U8,
  FLOAT,
  U16,
  U16_4
}GeglChannelDataType;

typedef union
{
  guint8 u8;
  gfloat f;
  guint16 u16;
  guint16 u16_4; 
}GeglChannelValue;

typedef struct _GeglRect GeglRect;

struct _GeglRect
{
  gint x;
  gint y;
  guint w;
  guint h;
};

typedef struct _GeglPoint GeglPoint;
struct _GeglPoint
{
  gint x;
  gint y;
};

#define ROUND(x) ((x)>0 ? (gint)((x)+.5) : (gint)((x)-.5))

#endif /* __GEGL_TYPES_H__ */
