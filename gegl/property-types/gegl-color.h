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
 * Copyright 2006 Martin Nordholts <enselic@hotmail.com>
 */

#ifndef __GEGL_COLOR_H__
#define __GEGL_COLOR_H__

#include <glib-object.h>

G_BEGIN_DECLS

#define GEGL_TYPE_COLOR            (gegl_color_get_type ())
#define GEGL_COLOR(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GEGL_TYPE_COLOR, GeglColor))
#define GEGL_COLOR_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  GEGL_TYPE_COLOR, GeglColorClass))
#define GEGL_IS_COLOR(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GEGL_TYPE_COLOR))
#define GEGL_IS_COLOR_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  GEGL_TYPE_COLOR))
#define GEGL_COLOR_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),  GEGL_TYPE_COLOR, GeglColorClass))

typedef struct _GeglColorClass  GeglColorClass;

struct _GeglColor
{
  GObject parent_instance;
};

struct _GeglColorClass
{
  GObjectClass parent_class;
};

GType        gegl_color_get_type               (void) G_GNUC_CONST;

GeglColor *  gegl_color_new                    (const gchar *string);

void         gegl_color_get_rgba               (GeglColor   *self,
                                                gfloat      *r,
                                                gfloat      *g,
                                                gfloat      *b,
                                                gfloat      *a);

void         gegl_color_set_rgba               (GeglColor   *self,
                                                gfloat       r,
                                                gfloat       g,
                                                gfloat       b,
                                                gfloat       a);

const gfloat*gegl_color_float4                 (GeglColor *color);

#define GEGL_TYPE_PARAM_COLOR           (gegl_param_color_get_type ())
#define GEGL_IS_PARAM_SPEC_COLOR(pspec) (G_TYPE_CHECK_INSTANCE_TYPE ((pspec), GEGL_TYPE_PARAM_COLOR))

GType        gegl_param_color_get_type         (void) G_GNUC_CONST;

GParamSpec * gegl_param_spec_color             (const gchar *name,
                                                const gchar *nick,
                                                const gchar *blurb,
                                                GeglColor   *default_color,
                                                GParamFlags  flags);

GParamSpec * gegl_param_spec_color_from_string (const gchar *name,
                                                const gchar *nick,
                                                const gchar *blurb,
                                                const gchar *default_color_string,
                                                GParamFlags  flags);

G_END_DECLS

#endif /* __GEGL_COLOR_H__ */
