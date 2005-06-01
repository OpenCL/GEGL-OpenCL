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

#ifndef __GEGL_NORMALIZER_H__
#define __GEGL_NORMALIZER_H__

#include "gegl-object.h"

G_BEGIN_DECLS


#define GEGL_TYPE_NORMALIZER            (gegl_normalizer_get_type ())
#define GEGL_NORMALIZER(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GEGL_TYPE_NORMALIZER, GeglNormalizer))
#define GEGL_NORMALIZER_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  GEGL_TYPE_NORMALIZER, GeglNormalizerClass))
#define GEGL_IS_NORMALIZER(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GEGL_TYPE_NORMALIZER))
#define GEGL_IS_NORMALIZER_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  GEGL_TYPE_NORMALIZER))
#define GEGL_NORMALIZER_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),  GEGL_TYPE_NORMALIZER, GeglNormalizerClass))


typedef struct _GeglNormalizer      GeglNormalizer;
typedef struct _GeglNormalizerClass GeglNormalizerClass;

struct _GeglNormalizer
{
  GeglObject  parent_instance;
};

struct _GeglNormalizerClass
{
  GeglObjectClass  parent_class;

  gdouble * (* normalize)   (const GeglNormalizer *self,
                             const gdouble        *unnor_data,
                             gdouble              *nor_data,
                             gint                  length,
                             gint                  stride);
  gdouble * (* unnormalize) (const GeglNormalizer *self,
                             const gdouble        *nor_data,
                             gdouble              *unnor_data,
                             gint                  length,
                             gint                  stride);
};


GType     gegl_normalizer_get_type    (void) G_GNUC_CONST;

gdouble * gegl_normalizer_normalize   (const GeglNormalizer *self,
                                       const gdouble        *unnor_data,
                                       gdouble              *nor_data,
                                       gint                  length,
                                       gint                  stride);
gdouble * gegl_normalizer_unnormalize (const GeglNormalizer *self,
                                       const gdouble        *nor_data,
                                       gdouble              *unnor_data,
                                       gint                  length,
                                       gint                  stride);


G_END_DECLS

#endif /* __GEGL_NORMALIZER_H__ */

