/* This file is part of GEGL
 *
 * GEGL is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * GEGL is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with GEGL; if not, see <http://www.gnu.org/licenses/>.
 *
 * Copyright 2003 Calvin Williamson
 *           2006 Øyvind Kolås
 */

#ifndef __GEGL_TYPES_INTERNAL_H__
#define __GEGL_TYPES_INTERNAL_H__

G_BEGIN_DECLS

typedef struct _GeglPad              GeglPad;
typedef struct _GeglConnection       GeglConnection;

typedef struct _GeglEvalManager      GeglEvalManager;
typedef struct _GeglDotVisitor       GeglDotVisitor;
typedef struct _GeglVisitable        GeglVisitable;
typedef struct _GeglVisitor          GeglVisitor;

typedef struct _GeglPoint            GeglPoint;
typedef struct _GeglDimension        GeglDimension;

struct _GeglPoint
{
  gint x;
  gint y;
};

struct _GeglDimension
{
  gint width;
  gint height;
};


static inline int gegl_level_from_scale (gfloat scale)
{
  gint level = 0;
  //gint factor = 1;

  while (scale <= 0.500001)
  {
    scale *= 2;
   // factor *= 2;
    level++;
#if 0
    if (rect)
    {
      rect->x = 0 < rect->x ? rect->x/2 : (rect->x - 1) / 2;
      rect->y = 0 < rect->y ? rect->y/2 : (rect->y - 1) / 2;
      rect->width = 0 < rect->width ? rect->width/2 : (rect->width  - 1) / 2;
      rect->height = 0 < rect->height ? rect->height/2 : (rect->height  - 1) / 2;
    }
#endif
  }

  return level;
}

G_END_DECLS

#endif /* __GEGL_TYPES_INTERNAL_H__ */
