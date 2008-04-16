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
 * Copyright 2006 Øyvind Kolås <pippin@gimp.org>
 */

#ifndef __GEGL_HANDLER_CACHE_H__
#define __GEGL_HANDLER_CACHE_H__

#include "gegl-handler.h"

#define GEGL_TYPE_HANDLER_CACHE            (gegl_handler_cache_get_type ())
#define GEGL_HANDLER_CACHE(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GEGL_TYPE_HANDLER_CACHE, GeglHandlerCache))
#define GEGL_HANDLER_CACHE_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  GEGL_TYPE_HANDLER_CACHE, GeglHandlerCacheClass))
#define GEGL_IS_HANDLER_CACHE(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GEGL_TYPE_HANDLER_CACHE))
#define GEGL_IS_HANDLER_CACHE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  GEGL_TYPE_HANDLER_CACHE))
#define GEGL_HANDLER_CACHE_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),  GEGL_TYPE_HANDLER_CACHE, GeglHandlerCacheClass))


typedef struct _GeglHandlerCache      GeglHandlerCache;
typedef struct _GeglHandlerCacheClass GeglHandlerCacheClass;

struct _GeglHandlerCache
{
  GeglHandler parent_instance;

/*  GQueue     *queue;
  gint        size;
  gint        wash_percentage;
  gint        hits;
  gint        misses;*/
};

struct _GeglHandlerCacheClass
{
  GeglHandlerClass parent_class;
};

GType gegl_handler_cache_get_type (void) G_GNUC_CONST;

#endif
