#ifndef __GEGL_TYPES_H__
#define __GEGL_TYPES_H__

#include <gtk/gtk.h>

typedef enum
{
  COLORSPACE_NONE,
  RGB,
  GRAY,
  INDEXED,
  CMYK,
  SRGB,
  CIE_XYZ
} GeglColorSpace;

typedef enum
{
  COMPOSITE_REPLACE,
  COMPOSITE_OVER,
  COMPOSITE_IN,
  COMPOSITE_OUT,
  COMPOSITE_ATOP,
  COMPOSITE_XOR,
  COMPOSITE_PLUS
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
  U16,
  FLOAT
}GeglChannelDataType;

typedef union
{
  guint8 u8;
  guint16 u16;
  gfloat f;
}GeglChannelValue;

typedef struct _GeglRect GeglRect;

struct _GeglRect
{
  gint x;
  gint y;
  guint w;
  guint h;
};

#endif /* __GEGL_TYPES_H__ */
