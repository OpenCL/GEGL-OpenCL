#ifndef __GEGL_TYPES_H__
#define __GEGL_TYPES_H__

#include <glib.h>
#include <glib-object.h>

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
