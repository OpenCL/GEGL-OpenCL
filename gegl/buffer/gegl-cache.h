/* This file is part of GEGL editor -- a gtk frontend for GEGL
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 * Copyright (C) 2003, 2004, 2006 Øyvind Kolås
 */

#ifndef __GEGL_PROJECTION_H__
#define __GEGL_PROJECTION_H__

#include "gegl.h"
#include "gegl-buffer.h"

G_BEGIN_DECLS

#define GEGL_TYPE_PROJECTION            (gegl_projection_get_type ())
#define GEGL_PROJECTION(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GEGL_TYPE_PROJECTION, GeglProjection))
#define GEGL_PROJECTION_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  GEGL_TYPE_PROJECTION, GeglProjectionClass))
#define GEGL_IS_PROJECTION(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GEGL_TYPE_PROJECTION))
#define GEGL_IS_PROJECTION_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  GEGL_TYPE_PROJECTION))
#define GEGL_PROJECTION_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),  GEGL_TYPE_PROJECTION, GeglProjectionClass))

typedef struct _GeglProjection       GeglProjection;
typedef struct _GeglProjectionClass GeglProjectionClass;

struct _GeglProjection
{
  GeglBuffer    parent;
  GeglNode     *node;
  void         *format;

  GeglRegion   *valid_region;
  GeglRegion   *queued_region;
  
  /*< private >*/
  GList        *dirty_rects;

  guint         render_id;
  guint         monitor_id;
};

struct _GeglProjectionClass
{
  GeglBufferClass buffer_class;
};

GType  gegl_projection_get_type    (void) G_GNUC_CONST;

void   gegl_projection_update_rect     (GeglProjection *self,
                                        GeglRectangle  roi);

void   gegl_projection_forget          (GeglProjection *self,
                                        GeglRectangle  *roi);

void   gegl_projection_forget_queue    (GeglProjection *self,
                                        GeglRectangle  *roi);

gboolean gegl_projection_render        (GeglProjection *self);

G_END_DECLS

#endif /* __GEGL_PROJECTION_H__ */
