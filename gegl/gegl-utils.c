#include "gegl-utils.h"

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


