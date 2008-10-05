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
 * Copyright 2007 Øyvind Kolås <pippin@gimp.org>
 */

#ifndef __GEGL_VECTOR_H__
#define __GEGL_VECTOR_H__

#include <glib-object.h>

G_BEGIN_DECLS

#define GEGL_TYPE_VECTOR            (gegl_vector_get_type ())
#define GEGL_VECTOR(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GEGL_TYPE_VECTOR, GeglVector))
#define GEGL_VECTOR_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  GEGL_TYPE_VECTOR, GeglVectorClass))
#define GEGL_IS_VECTOR(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GEGL_TYPE_VECTOR))
#define GEGL_IS_VECTOR_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  GEGL_TYPE_VECTOR))
#define GEGL_VECTOR_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),  GEGL_TYPE_VECTOR, GeglVectorClass))

typedef struct _GeglVectorClass  GeglVectorClass;

struct _GeglVector
{
  GObject parent_instance;
};

struct _GeglVectorClass
{
  GObjectClass parent_class;
};

GType        gegl_vector_get_type       (void) G_GNUC_CONST;

GeglVector * gegl_vector_new            (void);


void         gegl_vector_line_to        (GeglVector  *self,
                                         gdouble      x,
                                         gdouble      y);

void         gegl_vector_move_to        (GeglVector *self,
                                         gdouble     x,
                                         gdouble     y);

void         gegl_vector_curve_to       (GeglVector *self,
                                         gdouble     x1,
                                         gdouble     y1,
                                         gdouble     x2,
                                         gdouble     y2,
                                         gdouble     x3,
                                         gdouble     y3);

void         gegl_vector_rel_line_to    (GeglVector  *self,
                                         gdouble      x,
                                         gdouble      y);

void         gegl_vector_rel_move_to    (GeglVector *self,
                                         gdouble     x,
                                         gdouble     y);

void         gegl_vector_rel_curve_to   (GeglVector *self,
                                         gdouble     x1,
                                         gdouble     y1,
                                         gdouble     x2,
                                         gdouble     y2,
                                         gdouble     x3,
                                         gdouble     y3);

void         gegl_vector_get_bounds     (GeglVector   *self,
                                         gdouble      *min_x,
                                         gdouble      *max_x,
                                         gdouble      *min_y,
                                         gdouble      *max_y);

gdouble      gegl_vector_get_length     (GeglVector  *self);


void         gegl_vector_calc           (GeglVector  *self,
                                         gdouble      pos,
                                         gdouble     *x,
                                         gdouble     *y);

void         gegl_vector_calc_values    (GeglVector  *self,
                                         guint        num_samples,
                                         gdouble     *xs,
                                         gdouble     *ys);


#define GEGL_TYPE_PARAM_VECTOR    (gegl_param_vector_get_type ())
#define GEGL_IS_PARAM_VECTOR(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GEGL_TYPE_PARAM_VECTOR))

GType        gegl_param_vector_get_type (void) G_GNUC_CONST;

GParamSpec * gegl_param_spec_vector     (const gchar *name,
                                         const gchar *nick,
                                         const gchar *blurb,
                                         GeglVector  *default_vector,
                                         GParamFlags  flags);

#include <gegl-buffer.h>

void gegl_vector_fill (GeglBuffer *buffer,
                       GeglVector *vector,
                       GeglColor  *color,
                       gboolean    winding);

void gegl_vector_stroke (GeglBuffer *buffer,
                         GeglVector *vector,
                         GeglColor  *color,
                         gdouble     linewidth,
                         gdouble     hardness);

void gegl_vector_parse_svg_path (GeglVector *vector,
                                 const gchar *path);

G_END_DECLS

#endif /* __GEGL_VECTOR_H__ */
