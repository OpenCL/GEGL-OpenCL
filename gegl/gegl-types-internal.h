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

#include <babl/babl.h>

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

  while (scale <= 0.500001)
  {
    scale *= 2;
    level++;
  }

  return level;
}

/* the code in babl for looking up models, formats and types is quick -
   but when formats end up being used as consts for comparisons in the core of
   GEGL, it is good to have even better caching, hence this way of generating
   and accessing per compilation unit caches of formats.
 */

#define GEGL_CACHED_BABL(klass, typ, name)  \
static inline const Babl *gegl_babl_##typ (void) { static const Babl *type = NULL; if (!type) type = babl_##klass (name); return type; }

GEGL_CACHED_BABL(type, half, "half")
GEGL_CACHED_BABL(type, float, "float")
GEGL_CACHED_BABL(type, u8, "u8")
GEGL_CACHED_BABL(type, u16, "u16")
GEGL_CACHED_BABL(type, u32, "u32")
GEGL_CACHED_BABL(type, double, "double")

GEGL_CACHED_BABL(format, rgba_float, "R'G'B'A float")
GEGL_CACHED_BABL(format, rgbA_float, "R'aG'aB'aA float")
GEGL_CACHED_BABL(format, rgba_linear_float, "RGBA float")
GEGL_CACHED_BABL(format, rgbA_linear_float, "RaGaBaA float")
GEGL_CACHED_BABL(format, ya_float, "Y'A float")
GEGL_CACHED_BABL(format, yA_float, "Y'aA float")
GEGL_CACHED_BABL(format, ya_linear_float, "Y float")
GEGL_CACHED_BABL(format, yA_linear_float, "YaA float")

G_END_DECLS

#endif /* __GEGL_TYPES_INTERNAL_H__ */
