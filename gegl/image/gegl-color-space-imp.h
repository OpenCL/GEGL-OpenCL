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

#ifndef GEGL_COLOR_SPACE_IMP_H
#define GEGL_COLOR_SPACE_IMP_H

#include "gegl-object.h"

#define GEGL_TYPE_COLOR_SPACE_IMP               (gegl_color_space_imp_get_type ())
#define GEGL_COLOR_SPACE_IMP(obj)               (G_TYPE_CHECK_INSTANCE_CAST ((obj), GEGL_TYPE_COLOR_SPACE_IMP, GeglColorSpaceImp))
#define GEGL_COLOR_SPACE_IMP_CLASS(klass)       (G_TYPE_CHECK_CLASS_CAST ((klass),  GEGL_TYPE_COLOR_SPACE_IMP, GeglColorSpaceImpClass))
#define GEGL_IS_COLOR_SPACE_IMP(obj)            (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GEGL_TYPE_COLOR_SPACE_IMP))
#define GEGL_IS_COLOR_SPACE_IMP_CLASS(klass)    (G_TYPE_CHECK_CLASS_TYPE ((klass),  GEGL_TYPE_COLOR_SPACE_IMP))
#define GEGL_COLOR_SPACE_IMP_GET_CLASS(obj)     (G_TYPE_INSTANCE_GET_CLASS ((obj),  GEGL_TYPE_COLOR_SPACE_IMP, GeglColorSpaceImpClass))

GType gegl_color_space_imp_get_type (void) G_GNUC_CONST;

typedef struct _GeglColorSpaceImp GeglColorSpaceImp;
struct _GeglColorSpaceImp
{
  GeglObject parent;
};

typedef struct _GeglColorSpaceImpClass GeglColorSpaceImpClass;
struct _GeglColorSpaceImpClass
{
  GeglObjectClass parent_class;
  GeglImage *(*convert_tile) (const GeglColorSpaceImp * imp, GeglTile * input,
			      const GeglColorSpace * destination_space,
			      GeglColorSpaceIntent intent);
  GeglColorSpace *(*create_from_profile) (const GeglColorSpaceImp * imp,
					  gchar * icc_filename);
  GeglColorSpace *(*create_from_profile_mem) (const GeglColorSpaceImp * imp,
					      void *profile);
  GeglColorSpace *(*create_from_id) (const GeglColorSpaceImp * imp,
				     gint colorspace_id);
};

GeglImage *gegl_color_space_imp_convert_image (const GeglColorSpaceImp * imp,
					       GeglImage * input,
					       const GeglColorSpace destination_space,
					       GeglColorSpaceIntent intent);
GeglColorSpace *gegl_color_space_imp_create_from_profile (const GeglColorSpaceImp * imp,
							  gchar * icc_filename);
GeglColorSpace *gegl_color_space_imp_create_from_profile_mem (
                                  const GeglColorSpaceImp * imp,
							      void *profile);
GeglColorSpace *gegl_color_space_imp_create_from_id (
                             const GeglColorSpaceImp * imp,
                             gint colorspace_id);

#endif
