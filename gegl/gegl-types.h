#ifndef __GEGL_TYPES_H__
#define __GEGL_TYPES_H__

#include <gtk/gtk.h>

typedef enum
{
  GEGL_COLORSPACE_NONE,
  GEGL_GRAY,
  GEGL_RGB,
  GEGL_CMYK,
  GEGL_XYZ,
  GEGL_HSV,
} GeglColorSpace;

typedef enum
{
  GEGL_COMPOSITE_REPLACE,
  GEGL_COMPOSITE_OVER,
  GEGL_COMPOSITE_IN,
  GEGL_COMPOSITE_OUT,
  GEGL_COMPOSITE_ATOP,
  GEGL_COMPOSITE_XOR
}GeglCompositeMode;

typedef enum
{
  GEGL_COLOR_WHITE,
  GEGL_COLOR_BLACK,
  GEGL_COLOR_RED,
  GEGL_COLOR_GREEN,
  GEGL_COLOR_BLUE,
  GEGL_COLOR_GRAY,
  GEGL_COLOR_HALF_WHITE,
  GEGL_COLOR_TRANSPARENT,
  GEGL_COLOR_WHITE_TRANSPARENT,
  GEGL_COLOR_BLACK_TRANSPARENT
} GeglColorConstant;

typedef enum
{
  GEGL_DATATYPE_NONE,
  GEGL_U8,
  GEGL_FLOAT,
  GEGL_U16,
  GEGL_U16_4
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
