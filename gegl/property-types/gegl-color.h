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

typedef struct _GeglColorClass   GeglColorClass;
typedef struct _GeglColorPrivate GeglColorPrivate;

struct _GeglColor
{
  GObject           parent_instance;
  GeglColorPrivate *priv;
};

struct _GeglColorClass
{
  GObjectClass parent_class;
};

GType        gegl_color_get_type               (void) G_GNUC_CONST;


/***
 * GeglColor:
 *
 * GeglColor is an object containing a color at the moment only RGB colors
 * are supported, in the future a GeglColor might also indicate other
 * enumerated or natively in other color representations colors.
 */

/**
 * gegl_color_new:
 * @string: a string describing the color to be created.
 *
 * Creates a new #GeglColor.
 *
 * Returns the newly created #GeglColor.
 */
GeglColor *  gegl_color_new                    (const gchar *string);

/**
 * gegl_color_get_rgba:
 * @color: a #GeglColor
 * @red: red return location.
 * @green: green return location.
 * @blue: blue return location.
 * @alpha: alpha return location.
 *
 * Retrieves the current set color as linear light non premultipled RGBA data,
 * any of the return pointers can be omitted.
 */
void         gegl_color_get_rgba               (GeglColor   *color,
                                                gdouble     *red,
                                                gdouble     *green,
                                                gdouble     *blue,
                                                gdouble     *alpha);

/**
 * gegl_color_set_rgba:
 * @color: a #GeglColor
 * @red: red value
 * @green: green value
 * @blue: blue value
 * @alpha: alpha value
 *
 * Retrieves the current set color as linear light non premultipled RGBA data
 */
void         gegl_color_set_rgba               (GeglColor   *color,
                                                gdouble      red,
                                                gdouble      green,
                                                gdouble      blue,
                                                gdouble      alpha);
/**
 * gegl_color_set_pixel:
 * @color: a #GeglColor
 * @format: a babl pixel format
 * @pixel: pointer to a pixel
 *
 * Set a GeglColor from a pointer to a pixel and it's babl format.
 */
void         gegl_color_set_pixel              (GeglColor   *color,
                                                const Babl  *format,
                                                const void  *pixel);
/**
 * gegl_color_get_pixel:
 * @color: a #GeglColor
 * @format: a babl pixel format
 * @pixel: pointer to a pixel
 *
 * Store the color in a pixel in the given format.
 */
void         gegl_color_get_pixel              (GeglColor   *color,
                                                const Babl  *format,
                                                void        *pixel);

/***
 */

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
