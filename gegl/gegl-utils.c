#include "gegl-utils.h"
#include "gegl-types.h"

#include "gegl-color-model-gray-u8.h"
#include "gegl-color-model-gray-float.h"
#include "gegl-color-model-gray-u16.h"
#include "gegl-color-model-gray-u16_4.h"
#include "gegl-color-model-rgb-u8.h"
#include "gegl-color-model-rgb-float.h"
#include "gegl-color-model-rgb-u16.h"
#include "gegl-color-model-rgb-u16_4.h"

void gegl_rect_set (GeglRect *r,
                  gint x,
                  gint y,
                  guint w,
                  guint h)
{
  r->x = x;
  r->y = y;
  r->w = w;
  r->h = h;
}

void gegl_rect_copy (GeglRect *to,
                   GeglRect *from)
{
  to->x = from->x;
  to->y = from->y;
  to->w = from->w;
  to->h = from->h;
}

GeglColorModel *
gegl_color_model_factory (GeglColorSpace space,
                          GeglChannelDataType data,
                          gboolean has_alpha)
{
  switch (space)
    {
    case GRAY:
      switch (data)
       {
       case U8:
         return GEGL_COLOR_MODEL(gegl_color_model_gray_u8_new(has_alpha));
       case FLOAT:
         return GEGL_COLOR_MODEL(gegl_color_model_gray_float_new(has_alpha));
       default:
         return NULL;
       } 
    case RGB:
      switch (data)
       {
       case U8:
         return GEGL_COLOR_MODEL(gegl_color_model_rgb_u8_new(has_alpha));
       case FLOAT:
         return GEGL_COLOR_MODEL(gegl_color_model_rgb_float_new(has_alpha));
       default:
         return NULL;
       } 
    default:
      return NULL;
    } 
}

