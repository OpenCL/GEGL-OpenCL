#ifndef __GEGL_TYPES_H__
#define __GEGL_TYPES_H__

#include <gtk/gtk.h>

typedef enum
{
  GEGL_COLOR_SPACE_NONE,
  GEGL_COLOR_SPACE_GRAY,
  GEGL_COLOR_SPACE_RGB
}GeglColorSpace;

typedef enum
{
  GEGL_COLOR_ALPHA_SPACE_NONE,
  GEGL_COLOR_ALPHA_SPACE_GRAY,
  GEGL_COLOR_ALPHA_SPACE_GRAYA,
  GEGL_COLOR_ALPHA_SPACE_RGB,
  GEGL_COLOR_ALPHA_SPACE_RGBA
}GeglColorAlphaSpace;

typedef enum
{
  GEGL_COLOR_MODEL_TYPE_NONE,
  GEGL_COLOR_MODEL_TYPE_GRAY_U8,
  GEGL_COLOR_MODEL_TYPE_GRAY_U16,
  GEGL_COLOR_MODEL_TYPE_GRAY_U16_4,
  GEGL_COLOR_MODEL_TYPE_GRAY_FLOAT,
  GEGL_COLOR_MODEL_TYPE_GRAYA_U8,
  GEGL_COLOR_MODEL_TYPE_GRAYA_U16,
  GEGL_COLOR_MODEL_TYPE_GRAYA_U16_4,
  GEGL_COLOR_MODEL_TYPE_GRAYA_FLOAT,
  GEGL_COLOR_MODEL_TYPE_RGB_U8,
  GEGL_COLOR_MODEL_TYPE_RGB_U16,
  GEGL_COLOR_MODEL_TYPE_RGB_U16_4,
  GEGL_COLOR_MODEL_TYPE_RGB_FLOAT,
  GEGL_COLOR_MODEL_TYPE_RGBA_U8,
  GEGL_COLOR_MODEL_TYPE_RGBA_U16,
  GEGL_COLOR_MODEL_TYPE_RGBA_U16_4,
  GEGL_COLOR_MODEL_TYPE_RGBA_FLOAT
}GeglColorModelType;

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
}GeglColorConstant;

typedef enum
{
  GEGL_NONE,
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

typedef struct _GeglDimension GeglDimension;
struct _GeglDimension
{
  gint width;
  gint height;
};

#define ROUND(x) ((x)>0 ? (gint)((x)+.5) : (gint)((x)-.5))

#endif /* __GEGL_TYPES_H__ */
