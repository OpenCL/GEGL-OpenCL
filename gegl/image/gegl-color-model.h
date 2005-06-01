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

#ifndef GEGL_COLOR_MODEL_H
#define GEGL_COLOR_MODEL_H


#define GEGL_TYPE_COLOR_MODEL               (gegl_color_model_get_type ())
#define GEGL_COLOR_MODEL(obj)               (G_TYPE_CHECK_INSTANCE_CAST ((obj), GEGL_TYPE_COLOR_MODEL, GeglColorModel))
#define GEGL_COLOR_MODEL_CLASS(klass)       (G_TYPE_CHECK_CLASS_CAST ((klass),  GEGL_TYPE_COLOR_MODEL, GeglColorModelClass))
#define GEGL_IS_COLOR_MODEL(obj)            (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GEGL_TYPE_COLOR_MODEL))
#define GEGL_IS_COLOR_MODEL_CLASS(klass)    (G_TYPE_CHECK_CLASS_TYPE ((klass),  GEGL_TYPE_COLOR_MODEL))
#define GEGL_COLOR_MODEL_GET_CLASS(obj)     (G_TYPE_INSTANCE_GET_CLASS ((obj),  GEGL_TYPE_COLOR_MODEL, GeglColorModelClass))

GType gegl_color_model_get_type (void) G_GNUC_CONST;

typedef struct _GeglColorModel GeglColorModel;
struct _GeglColorModel
{
  GeglObject parent;
  gint *color_channels;
  gint alpha_channel;
  gint num_colors;
  gboolean *is_lum_channel;
};

typedef struct _GeglColorModelClass GeglColorModelClass;
struct _GeglColorModelClass
{
  GeglObjectClass parent_class;
};


gint gegl_color_model_is_color_channel (const GeglColorModel * self,
					gint channel_num);
void gegl_color_model_set_color_channels (GeglColorModel * self,
					  gint * bands,
					  gboolean * has_lum_info,
					  gint num_colors);
gboolean gegl_color_model_has_alpha (const GeglColorModel * self);
gint gegl_color_model_get_alpha (const GeglColorModel * self);
void gegl_color_model_set_alpha (const GeglColorModel * self);
void gegl_color_model_remove_alpha (GeglColorModel * self);
const gint gegl_color_model_is_lum_channel (const GeglColorModel * self);
gint gegl_color_model_get_num_colors (const GeglColorModel * self);

#endif
