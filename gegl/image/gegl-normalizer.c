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

#include "config.h"

#include <glib-object.h>

#include "gegl-image-types.h"

#include "gegl-normalizer.h"

static void  gegl_normalizer_class_init (GeglNormalizerClass *klass);
static void  gegl_normalizer_init       (GeglNormalizer      *self);


G_DEFINE_ABSTRACT_TYPE (GeglNormalizer, gegl_normalizer, GEGL_TYPE_OBJECT)


static void
gegl_normalizer_class_init (GeglNormalizerClass *klass)
{
  klass->normalize   = NULL;
  klass->unnormalize = NULL;
}

static void
gegl_normalizer_init (GeglNormalizer *self)
{
}

gdouble *
gegl_normalizer_normalize (const GeglNormalizer * self,
			   const gdouble * unnor_data,
			   gdouble * nor_data, gint length, gint stride)
{
  GeglNormalizerClass *class = GEGL_NORMALIZER_GET_CLASS (self);
  g_return_val_if_fail (GEGL_IS_NORMALIZER (self), NULL);
  g_return_val_if_fail (unnor_data != NULL, NULL);
  g_return_val_if_fail (class->normalize != NULL, NULL);

  if (nor_data == NULL)
    {
      nor_data = g_new (gdouble, length);
    }
  return class->normalize (self, unnor_data, nor_data, length, stride);
}

gdouble *
gegl_normalizer_unnormalize (const GeglNormalizer *self,
			     const gdouble        *nor_data,
			     gdouble              *unnor_data,
                             gint                  length,
                             gint                  stride)
{
  GeglNormalizerClass *class;

  g_return_val_if_fail (GEGL_IS_NORMALIZER (self), NULL);
  g_return_val_if_fail (unnor_data != NULL, NULL);

  class = GEGL_NORMALIZER_GET_CLASS (self);

  g_return_val_if_fail (class->unnormalize != NULL, NULL);

  if (! nor_data)
    nor_data = g_new (gdouble, length);

  return class->unnormalize (self, nor_data, unnor_data, length, stride);
}
