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
 * Copyright 2007 Mark Probst <mark.probst@gmail.com>
 */

#ifndef __GEGL_CURVE_H__
#define __GEGL_CURVE_H__

#include <glib-object.h>

G_BEGIN_DECLS

#define GEGL_TYPE_CURVE            (gegl_curve_get_type ())
#define GEGL_CURVE(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GEGL_TYPE_CURVE, GeglCurve))
#define GEGL_CURVE_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  GEGL_TYPE_CURVE, GeglCurveClass))
#define GEGL_IS_CURVE(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GEGL_TYPE_CURVE))
#define GEGL_IS_CURVE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  GEGL_TYPE_CURVE))
#define GEGL_CURVE_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),  GEGL_TYPE_CURVE, GeglCurveClass))

typedef struct _GeglCurveClass  GeglCurveClass;

struct _GeglCurve
{
  GObject parent_instance;
};

struct _GeglCurveClass
{
  GObjectClass parent_class;
};

GType        gegl_curve_get_type       (void) G_GNUC_CONST;

GeglCurve  * gegl_curve_new            (gdouble      y_min,
                                        gdouble      y_max);

GeglCurve  * gegl_curve_default_curve  (void) G_GNUC_CONST;

void         gegl_curve_get_y_bounds   (GeglCurve   *self,
                                        gdouble     *min_y,
                                        gdouble     *max_y);
/* should perhaps become: */
void         gegl_curve_get_bounds     (GeglCurve   *self,
                                        gdouble     *min_x,
                                        gdouble     *max_x,
                                        gdouble     *min_y,
                                        gdouble     *max_y);

guint        gegl_curve_add_point      (GeglCurve   *self,
                                        gdouble      x,
                                        gdouble      y);

void         gegl_curve_get_point      (GeglCurve   *self,
                                        guint        index,
                                        gdouble     *x,
                                        gdouble     *y);

void         gegl_curve_set_point      (GeglCurve   *self,
                                        guint        index,
                                        gdouble      x,
                                        gdouble      y);

guint        gegl_curve_num_points     (GeglCurve   *self);

gdouble      gegl_curve_calc_value     (GeglCurve   *self,
                                        gdouble      x);

void         gegl_curve_calc_values    (GeglCurve   *self,
                                        gdouble      x_min,
                                        gdouble      x_max,
                                        guint        num_samples,
                                        gdouble     *xs,
                                        gdouble     *ys);


#define GEGL_TYPE_PARAM_CURVE           (gegl_param_curve_get_type ())
#define GEGL_IS_PARAM_SPEC_CURVE(pspec) (G_TYPE_CHECK_INSTANCE_TYPE ((pspec), GEGL_TYPE_PARAM_CURVE))

GType        gegl_param_curve_get_type (void) G_GNUC_CONST;

GParamSpec * gegl_param_spec_curve     (const gchar *name,
                                        const gchar *nick,
                                        const gchar *blurb,
                                        GeglCurve   *default_curve,
                                        GParamFlags  flags);

G_END_DECLS

#endif /* __GEGL_CURVE_H__ */
