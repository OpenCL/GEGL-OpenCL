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

#ifndef GEGL_TYPE_CURVE
#define GEGL_TYPE_CURVE            (gegl_curve_get_type ())
#endif
#define GEGL_CURVE(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GEGL_TYPE_CURVE, GeglCurve))
#define GEGL_CURVE_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  GEGL_TYPE_CURVE, GeglCurveClass))
#define GEGL_IS_CURVE(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GEGL_TYPE_CURVE))
#define GEGL_IS_CURVE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  GEGL_TYPE_CURVE))
#define GEGL_CURVE_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),  GEGL_TYPE_CURVE, GeglCurveClass))

typedef struct _GeglCurveClass  GeglCurveClass;

struct _GeglCurve
{
  GObject parent;
};

struct _GeglCurveClass
{
  GObjectClass parent;
};

GeglCurve*   gegl_curve_new		       (gfloat	     y_min,
						gfloat	     y_max);

GeglCurve*   gegl_curve_default_curve          (void) G_GNUC_CONST;

GType	     gegl_curve_get_type	       (void) G_GNUC_CONST;

void         gegl_curve_get_y_bounds           (GeglCurve    *self,
						gfloat       *min_y,
						gfloat       *max_y);

guint	     gegl_curve_add_point	       (GeglCurve    *self,
						gfloat       x,
						gfloat       y);

void	     gegl_curve_get_point	       (GeglCurve    *self,
						guint	     index,
						gfloat	     *x,
						gfloat	     *y);

void	     gegl_curve_set_point	       (GeglCurve    *self,
						guint	     index,
						gfloat	     x,
						gfloat	     y);

guint	     gegl_curve_num_points	       (GeglCurve   *self);

gfloat	     gegl_curve_calc_value	       (GeglCurve   *self,
						gfloat      x);

void	     gegl_curve_calc_values	       (GeglCurve   *self,
						gfloat	    x_min,
						gfloat      x_max,
						guint       num_samples,
						gfloat      *xs,
						gfloat      *ys);

GParamSpec * gegl_param_spec_curve             (const gchar *name,
                                                const gchar *nick,
                                                const gchar *blurb,
                                                GeglCurve   *default_curve,
                                                GParamFlags  flags);

GType        gegl_param_curve_get_type         (void) G_GNUC_CONST;

G_END_DECLS

#endif /* __GEGL_CURVE_H__ */
