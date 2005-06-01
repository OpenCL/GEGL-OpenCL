/*
 *   This file is part of GEGL.
 *
 *    GEGL is free software; you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License as published by
 *    the Free Software Foundation; either version 2 of the License, or
 *    (at your option) any later version.
 *
 *    GEGL is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with GEGL; if not, write to the Free Software
 *    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 *  Copyright 2003 Daniel S. Rogers
 *
 */

#ifndef GEGL_COLOR_SPACE_H
#define GEGL_COLOR_SPACE_H

#define GEGL_TYPE_COLOR_SPACE               (gegl_color_space_get_type ())
#define GEGL_COLOR_SPACE(obj)               (G_TYPE_CHECK_INSTANCE_CAST ((obj), GEGL_TYPE_COLOR_SPACE, GeglColorSpace))
#define GEGL_COLOR_SPACE_CLASS(klass)       (G_TYPE_CHECK_CLASS_CAST ((klass),  GEGL_TYPE_COLOR_SPACE, GeglColorSpaceClass))
#define GEGL_IS_COLOR_SPACE(obj)            (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GEGL_TYPE_COLOR_SPACE))
#define GEGL_IS_COLOR_SPACE_CLASS(klass)    (G_TYPE_CHECK_CLASS_TYPE ((klass),  GEGL_TYPE_COLOR_SPACE))
#define GEGL_COLOR_SPACE_GET_CLASS(obj)     (G_TYPE_INSTANCE_GET_CLASS ((obj),  GEGL_TYPE_COLOR_SPACE, GeglColorSpaceClass))

GType gegl_color_space_get_type (void) G_GNUC_CONST;

enum GeglColorSpaceIntent
{
  GEGL_INTENT_PERCEPTUAL,
  GEGL_INTENT_SATURATION,
  GEGL_INTENT_ABSOLUTE_COLORIMETRIC,
  GEGL_INTENT_RELATIVE_COLORIMETRIC
};

/*
 * This is intended to describe different archtypal color models
 * They could be named better
 */
enum GeglColorSpaceVarient
{
  GEGL_VARIENT_ADDITIVE,
  GEGL_VARIENT_SUBTRACTIVE,
  GEGL_VARIENT_LAB
};

typedef struct _GeglColorSpace GeglColorSpace;
struct _GeglColorSpace
{
  GeglObject parent;
  void *implementation_data;
  gchar *name;
  gint num_colors;
  gint varient;
  gdouble **limits;
};

typedef struct _GeglColorSpaceClass GeglColorSpaceClass;
struct _GeglColorSpaceClass
{
  GeglObjectClass parent_class;
  GeglColorSpaceImp *implementation;
};

GeglColorSpace *gegl_color_space_create_icc (gchar * icc_filename);
GeglColorSpace *gegl_color_space_create_sRGB (void);
GeglColorSpace *gegl_color_space_create_SWOP (void);
GeglColorSpace *gegl_color_space_create_grey22 (void);
GeglColorSpace *gegl_color_space_create_grey (gdouble gamma);
GeglColorSpace *gegl_color_space_create_lab (void);
gboolean gegl_color_space_check_color_model (void);


#endif
