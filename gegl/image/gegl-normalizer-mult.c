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

#include "gegl-normalizer-mult.h"


enum
{
  PROP_0,
  PROP_ALPHA,
  PROP_LAST
};

static void     class_init    (gpointer              g_class,
                               gpointer              class_data);
static void     instance_init (GTypeInstance        *instance,
                               gpointer              g_class);
static void     get_property  (GObject              *object,
                               guint                 property_id,
                               GValue               *value,
                               GParamSpec           *pspec);
static void     set_property  (GObject              *object,
                               guint                 property_id,
                               const GValue         *value,
                               GParamSpec           *pspec);
static gdouble *normalize     (const GeglNormalizer *self,
                               const gdouble        *unnor_data,
                               gdouble              *nor_data,
                               gint                  length,
                               gint                  stride);
static gdouble *unnormalize   (const GeglNormalizer *self,
                               const gdouble        *nor_data,
                               gdouble              *unnor_data,
                               gint                  length,
                               gint                  stride);


GType
gegl_normalizer_mult_get_type (void)
{
  static GType type = 0;
  if (!type)
    {
      static const GTypeInfo typeInfo = {
	/* interface types, classed types, instantiated types */
	sizeof (GeglNormalizerMultClass),
	NULL,			/* base_init */
	NULL,			/* base_finalize */

	/* classed types, instantiated types */
	class_init,		/* class_init */
	NULL,			/* class_finalize */
	NULL,			/* class_data */

	/* instantiated types */
	sizeof (GeglNormalizerMult),
	0,			/* n_preallocs */
	instance_init,		/* instance_init */

	/* value handling */
	NULL			/* value_table */
      };

      type = g_type_register_static (GEGL_TYPE_NORMALIZER,
				     "GeglNormalizerMult", &typeInfo, 0);
    }
  return type;
}

static void
class_init (gpointer g_class, gpointer class_data)
{
  GObjectClass *object_class = (GObjectClass *) g_class;
  GeglNormalizerClass *class = (GeglNormalizerClass *) g_class;
  class->normalize = normalize;
  class->unnormalize = unnormalize;

  object_class->get_property = get_property;
  object_class->set_property = set_property;

  g_object_class_install_property (object_class, PROP_ALPHA,
				   g_param_spec_double ("alpha",
							"Multiplier",
							"The multiplier used to rescale the sample",
							G_MINDOUBLE,
							G_MAXDOUBLE,
							1,
							G_PARAM_CONSTRUCT |
							G_PARAM_READWRITE));
  return;
}

static void
get_property (GObject * object,
	      guint property_id, GValue * value, GParamSpec * pspec)
{
  GeglNormalizerMult *self = (GeglNormalizerMult *) object;

  switch (property_id)
    {
    case PROP_ALPHA:
      g_value_set_double (value, self->alpha);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
set_property (GObject * object,
	      guint property_id, const GValue * value, GParamSpec * pspec)
{
  GeglNormalizerMult *self = (GeglNormalizerMult *) object;

  switch (property_id)
    {
    case PROP_ALPHA:
      self->alpha = g_value_get_double (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
instance_init (GTypeInstance * instance, gpointer g_class)
{
  GeglNormalizerMult *self = (GeglNormalizerMult *) instance;
  self->alpha = 0.0;
}

static gdouble *
normalize (const GeglNormalizer * normalizer,
	   const gdouble * unnor_data,
	   gdouble * nor_data, gint length, gint stride)
{
  GeglNormalizerMult *self = (GeglNormalizerMult *) normalizer;
  gdouble *nor_i = nor_data;
  const gdouble *unnor_i = unnor_data;
  int i;
  for (i = 0; i < length; i++)
    {
      (*nor_i) = (*unnor_i) * (self->alpha);
      nor_i += stride;
      unnor_i += stride;
    }
  return nor_data;
}

static gdouble *
unnormalize (const GeglNormalizer * normalizer,
	     const gdouble * nor_data,
	     gdouble * unnor_data, gint length, gint stride)
{
  GeglNormalizerMult *self = (GeglNormalizerMult *) normalizer;
  double alpha_inv = 1 / (self->alpha);
  const gdouble *nor_i = nor_data;
  gdouble *unnor_i = unnor_data;
  int i;
  for (i = 0; i < length; i++)
    {
      (*unnor_i) = (*nor_i) * alpha_inv;
      unnor_i += stride;
      nor_i += stride;
    }
  return unnor_data;
}
