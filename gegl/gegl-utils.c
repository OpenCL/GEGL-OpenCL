#include "gegl-utils.h"
#include "gegl-types.h"

#include "gegl-color-model-gray-u8.h"
#include "gegl-color-model-gray-float.h"
#include "gegl-color-model-rgb-u8.h"
#include "gegl-color-model-rgb-float.h"

void 
gegl_rect_set (GeglRect *r,
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

void 
gegl_rect_union (GeglRect *dest,
                 GeglRect *src1,
                 GeglRect *src2)
{
  gint x1 = MIN(src1->x, src2->x); 
  gint x2 = MAX(src1->x + src1->w, src2->x + src2->w);  
  gint y1 = MIN(src1->y, src2->y); 
  gint y2 = MAX(src1->y + src1->h, src2->y + src2->h);  
    
  dest->x = x1;
  dest->y = y1; 
  dest->w = x2 - x1;
  dest->h = y2 - y1;
}

gboolean 
gegl_rect_intersect(GeglRect *dest,
                    GeglRect *src1,
                    GeglRect *src2)
{
  gint x1, x2, y1, y2; 
    
  x1 = MAX(src1->x, src2->x); 
  x2 = MIN(src1->x + src1->w, src2->x + src2->w);  

  if (x2 <= x1)
    {
      gegl_rect_set (dest,0,0,0,0);
      return FALSE;
    }

  y1 = MAX(src1->y, src2->y); 
  y2 = MIN(src1->y + src1->h, src2->y + src2->h);  

  if (y2 <= y1)
    {
      gegl_rect_set (dest,0,0,0,0);
      return FALSE;
    }

  dest->x = x1;
  dest->y = y1; 
  dest->w = x2 - x1;
  dest->h = y2 - y1;
  return TRUE;
}

void 
gegl_rect_copy (GeglRect *to,
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

