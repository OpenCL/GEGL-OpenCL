#ifndef __GEGL_TYPES_H__
#define __GEGL_TYPES_H__

#include <glib.h>
#include <glib-object.h>

#define GEGL_DEFAULT_WIDTH 64 
#define GEGL_DEFAULT_HEIGHT 64 

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
  GEGL_ALPHA_NONE    = 0, 
  GEGL_A_ALPHA       = 1 << 0,
  GEGL_B_ALPHA       = 1 << 1,
  GEGL_A_B_ALPHA     = GEGL_A_ALPHA | GEGL_B_ALPHA
} GeglAlphaFlags;

typedef enum
{
  GEGL_NO_ALPHA  = 0, 
  GEGL_FG_ALPHA       = 1 << 0,
  GEGL_BG_ALPHA       = 1 << 1,
  GEGL_FG_BG_ALPHA    = GEGL_FG_ALPHA | GEGL_BG_ALPHA
} GeglFgBgAlphaFlags;

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
  gint w;
  gint h;
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
