/* This file is part of GEGL.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, see <http://www.gnu.org/licenses/>.
 *
 * Copyright (C) 2003, 2004, 2006 Øyvind Kolås
 */

#ifndef __GEGL_CACHE_H__
#define __GEGL_CACHE_H__

#include "gegl-buffer.h"
#include "gegl-buffer-private.h"

G_BEGIN_DECLS

#define GEGL_TYPE_CACHE            (gegl_cache_get_type ())
#define GEGL_CACHE(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GEGL_TYPE_CACHE, GeglCache))
#define GEGL_CACHE_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  GEGL_TYPE_CACHE, GeglCacheClass))
#define GEGL_IS_CACHE(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GEGL_TYPE_CACHE))
#define GEGL_IS_CACHE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  GEGL_TYPE_CACHE))
#define GEGL_CACHE_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),  GEGL_TYPE_CACHE, GeglCacheClass))

typedef struct _GeglCacheClass GeglCacheClass;

#define GEGL_CACHE_VALID_MIPMAPS 8

struct _GeglCache
{
  GeglBuffer    parent_instance;

  GeglRegion   *valid_region[GEGL_CACHE_VALID_MIPMAPS];
  GMutex        mutex;
};

struct _GeglCacheClass
{
  GeglBufferClass  parent_class;
};

GType    gegl_cache_get_type    (void) G_GNUC_CONST;
void     gegl_cache_invalidate  (GeglCache           *self,
                                 const GeglRectangle *roi);
void     gegl_cache_computed    (GeglCache           *self,
                                 const GeglRectangle *rect,
                                 gint                 level);

G_END_DECLS

#endif /* __GEGL_CACHE_H__ */
