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

/*
 * TODO  Fix the (get)|(set)_(sample)|(pixel)_normalized functions
 * The static versions should call the class pointers directly and
 * the non-static versions should contain all the error checking of the
 * arguments.
 */

#include <glib-object.h>
#include "gegl-sample-model.h"
#include "gegl-normalizer.h"

static gpointer parent_class;

enum
{
  PROP_0,
  PROP_WIDTH,
  PROP_HEIGHT,
  PROP_NUM_BANDS,
  PROP_NORMALIZERS,
  PROP_LAST
};

static void instance_init (GTypeInstance * instance, gpointer g_class);
static void class_init (gpointer g_class, gpointer class_data);
static void get_property (GObject * object,
			  guint property_id,
			  GValue * value, GParamSpec * pspec);
static void set_property (GObject * object,
			  guint property_id,
			  const GValue * value, GParamSpec * pspec);
static gdouble *get_pixel_double_default (const GeglSampleModel * self,
					  gint x,
					  gint y,
					  gdouble * dArray,
					  const GeglBuffer * buffer);
static void
set_pixel_double_default (const GeglSampleModel * self,
			  gint x,
			  gint y,
			  const gdouble * dArray, GeglBuffer * buffer);
static gdouble get_sample_normalized (const GeglSampleModel * self,
				      gint x,
				      gint y,
				      gint band, const GeglBuffer * buffer);
static void set_sample_normalized (const GeglSampleModel * self,
				   gint x,
				   gint y,
				   gint band,
				   gdouble sample, GeglBuffer * buffer);
static gdouble *get_pixel_normalized (const GeglSampleModel * self,
				      gint x,
				      gint y,
				      gdouble * d_array,
				      const GeglBuffer * buffer);
static void set_pixel_normalized (const GeglSampleModel * self,
				  gint x,
				  gint y,
				  const gdouble * d_array,
				  GeglBuffer * buffer);
static GeglNormalizer *get_normalizer (const GeglSampleModel * self,
				       gint band);
GType
gegl_sample_model_get_type (void)
{
  static GType type = 0;
  if (!type)
    {
      static const GTypeInfo typeInfo = {
	/* interface types, classed types, instantiated types */
	sizeof (GeglSampleModelClass),
	NULL,			/* base_init */
	NULL,			/* base_finalize */

	/* classed types, instantiated types */
	class_init,		/* class_init */
	NULL,			/* class_finalize */
	NULL,			/* class_data */

	/* instantiated types */
	sizeof (GeglSampleModel),
	0,			/* n_preallocs */
	instance_init,		/* instance_init */

	/* value handling */
	NULL			/* value_table */
      };

      type = g_type_register_static (GEGL_TYPE_OBJECT,
				     "GeglSampleModel",
				     &typeInfo, G_TYPE_FLAG_ABSTRACT);
    }
  return type;
}

static void
instance_init (GTypeInstance * instance, gpointer g_class)
{
  GeglSampleModel *sample_model = (GeglSampleModel *) instance;

  sample_model->width = 0;
  sample_model->height = 0;
  sample_model->num_bands = 0;
}

static void
class_init (gpointer g_class, gpointer class_data)
{
  GeglSampleModelClass *sm_class = (GeglSampleModelClass *) g_class;
  GObjectClass *object_class = (GObjectClass *) g_class;

  sm_class->get_pixel_double = get_pixel_double_default;
  sm_class->set_pixel_double = set_pixel_double_default;
  sm_class->get_sample_normalized = get_sample_normalized;
  sm_class->set_sample_normalized = set_sample_normalized;
  sm_class->get_pixel_normalized = get_pixel_normalized;
  sm_class->set_pixel_normalized = set_pixel_normalized;

  object_class->get_property = get_property;
  object_class->set_property = set_property;

  g_object_class_install_property (object_class, PROP_WIDTH,
				   g_param_spec_int ("width",
						     "Width",
						     "Width of the buffer described by this sample model",
						     0,
						     G_MAXINT,
						     0,
						     G_PARAM_CONSTRUCT_ONLY |
						     G_PARAM_READWRITE));
  g_object_class_install_property (object_class, PROP_HEIGHT,
				   g_param_spec_int ("height",
						     "Height",
						     "Height of the buffer described by this sample model",
						     0,
						     G_MAXINT,
						     0,
						     G_PARAM_CONSTRUCT_ONLY |
						     G_PARAM_READWRITE));

  g_object_class_install_property (object_class, PROP_NUM_BANDS,
				   g_param_spec_int ("num_bands",
						     "Number of Bands",
						     "Number of Bands in the buffer described by this sample model",
						     0,
						     G_MAXINT,
						     0,
						     G_PARAM_CONSTRUCT_ONLY |
						     G_PARAM_READWRITE));
  g_object_class_install_property (object_class, PROP_NORMALIZERS,
				   g_param_spec_pointer ("normalizers",
							 "Normalizer Array",
							 "Pointer to a GArray of Normalizer objects for each band",
							 G_PARAM_CONSTRUCT_ONLY
							 |
							 G_PARAM_READWRITE));


  parent_class = g_type_class_peek_parent (g_class);

}

static void
get_property (GObject * gobject, guint property_id, GValue * value,
	      GParamSpec * pspec)
{
  GeglSampleModel *self = (GeglSampleModel *) gobject;
  switch (property_id)
    {
    case PROP_WIDTH:
      g_value_set_int (value, gegl_sample_model_get_width (self));
      return;
    case PROP_HEIGHT:
      g_value_set_int (value, gegl_sample_model_get_height (self));
      return;
    case PROP_NUM_BANDS:
      g_value_set_int (value, gegl_sample_model_get_num_bands (self));
      return;
    case PROP_NORMALIZERS:
      g_value_set_pointer (value, self->normalizers);
      return;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (gobject, property_id, pspec);
      break;
    }

}

static void
set_property (GObject * object, guint property_id, const GValue * value,
	      GParamSpec * pspec)
{
  GeglSampleModel *self = (GeglSampleModel *) object;
  switch (property_id)
    {
    case PROP_WIDTH:
      self->width = g_value_get_int (value);
      return;
    case PROP_HEIGHT:
      self->height = g_value_get_int (value);
      return;
    case PROP_NUM_BANDS:
      self->num_bands = g_value_get_int (value);
      return;
    case PROP_NORMALIZERS:
      self->normalizers = g_value_get_pointer (value);
      return;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

gint
gegl_sample_model_get_num_bands (const GeglSampleModel * self)
{
  g_return_val_if_fail (GEGL_IS_SAMPLE_MODEL (self), 0);

  return self->num_bands;
}

gint
gegl_sample_model_get_width (const GeglSampleModel * self)
{
  g_return_val_if_fail (GEGL_IS_SAMPLE_MODEL (self), 0);

  return self->width;
}

gint
gegl_sample_model_get_height (const GeglSampleModel * self)
{
  g_return_val_if_fail (GEGL_IS_SAMPLE_MODEL (self), 0);

  return self->height;
}

gdouble *
gegl_sample_model_get_pixel_double (const GeglSampleModel * self,
				    gint x,
				    gint y,
				    gdouble * dArray,
				    const GeglBuffer * buffer)
{
  g_return_val_if_fail (GEGL_IS_SAMPLE_MODEL (self), NULL);
  g_return_val_if_fail (dArray != NULL, NULL);
  g_return_val_if_fail (GEGL_IS_BUFFER (buffer), NULL);
  g_return_val_if_fail (x < (self->width), NULL);
  g_return_val_if_fail (x >= 0, NULL);
  g_return_val_if_fail (y < (self->height), NULL);
  g_return_val_if_fail (y >= 0, NULL);

  GeglSampleModelClass *klass = GEGL_SAMPLE_MODEL_GET_CLASS (self);

  g_return_val_if_fail (klass->get_pixel_double, NULL);

  return klass->get_pixel_double (self, x, y, dArray, buffer);
}

static gdouble *
get_pixel_double_default (const GeglSampleModel * self,
			  gint x,
			  gint y, gdouble * dArray, const GeglBuffer * buffer)
{
  int i = 0;
  gdouble *dArray_pos = dArray;
  for (i = 0; i < self->num_bands; i++)
    {
      *dArray_pos =
	gegl_sample_model_get_sample_double (self, x, y, i, buffer);
      dArray_pos++;
    }
  return dArray;
}

void
gegl_sample_model_set_pixel_double (const GeglSampleModel * self,
				    gint x,
				    gint y,
				    const gdouble * dArray,
				    GeglBuffer * buffer)
{
  g_return_if_fail (GEGL_IS_SAMPLE_MODEL (self));
  g_return_if_fail (dArray != NULL);
  g_return_if_fail (GEGL_IS_BUFFER (buffer));
  g_return_if_fail (x < (self->width));
  g_return_if_fail (x >= 0);
  g_return_if_fail (y < (self->height));
  g_return_if_fail (y >= 0);

  GeglSampleModelClass *klass = GEGL_SAMPLE_MODEL_GET_CLASS (self);

  g_return_if_fail (klass->get_pixel_double);

  klass->set_pixel_double (self, x, y, dArray, buffer);
}

static void
set_pixel_double_default (const GeglSampleModel * self,
			  gint x,
			  gint y, const gdouble * dArray, GeglBuffer * buffer)
{
  /* already bounds check the arguments in the non-static accessor. */
  int i = 0;
  for (i = 0; i < self->num_bands; i++)
    {
      gegl_sample_model_set_sample_double (self, x, y, i, *dArray, buffer);
      dArray++;
    }
  return;
}

static gdouble
get_sample_normalized (const GeglSampleModel * self,
		       gint x, gint y, gint band, const GeglBuffer * buffer)
{
  GeglNormalizer *normalizer = get_normalizer (self, band);
  gdouble sample =
    gegl_sample_model_get_sample_double (self, x, y, band, buffer);
  gegl_normalizer_normalize (normalizer, &sample, &sample, 1, 1);
  return sample;
}

static void
set_sample_normalized (const GeglSampleModel * self,
		       gint x,
		       gint y, gint band, gdouble sample, GeglBuffer * buffer)
{
  GeglNormalizer *normalizer = get_normalizer (self, band);
  if (normalizer != NULL)
    {
      gegl_normalizer_unnormalize (normalizer, &sample, &sample, 1, 1);
    }
  gegl_sample_model_set_sample_double (self, x, y, band, sample, buffer);
  return;
}

static gdouble *
get_pixel_normalized (const GeglSampleModel * self,
		      gint x,
		      gint y, gdouble * nor_data, const GeglBuffer * buffer)
{
  int i = 0;
  if (nor_data == NULL)
    {
      nor_data = g_new (gdouble, self->num_bands);
    }
  gegl_sample_model_get_pixel_double (self, x, y, nor_data, buffer);
  for (i = 0; i < self->num_bands; i++)
    {
      GeglNormalizer *normalizer = get_normalizer (self, i);
      if (normalizer != NULL)
	{
	  gegl_normalizer_normalize (normalizer, nor_data, nor_data, 1,
				     self->num_bands);
	}
    }
  return nor_data;
}

static void
set_pixel_normalized (const GeglSampleModel * self,
		      gint x,
		      gint y, const gdouble * nor_data, GeglBuffer * buffer)
{
  int i = 0;
  gdouble unnor_data[self->num_bands];

  if (self->normalizers != NULL)
    {
      for (i = 0; i < self->num_bands; i++)
	{
	  GeglNormalizer *normalizer = get_normalizer (self, i);
	  gegl_normalizer_unnormalize (normalizer, nor_data, unnor_data, 1,
				       self->num_bands);
	}
      gegl_sample_model_set_pixel_double (self, x, y, unnor_data, buffer);
    }
  else
    {
      gegl_sample_model_set_pixel_double (self, x, y, nor_data, buffer);
    }
}

gdouble
gegl_sample_model_get_sample_double (const GeglSampleModel * self,
				     gint x,
				     gint y,
				     gint band, const GeglBuffer * buffer)
{
  g_return_val_if_fail (GEGL_IS_SAMPLE_MODEL (self), 0.0);
  g_return_val_if_fail (GEGL_IS_BUFFER (buffer), 0.0);
  g_return_val_if_fail (x < (self->width), 0.0);
  g_return_val_if_fail (x >= 0, 0.0);
  g_return_val_if_fail (y < (self->height), 0.0);
  g_return_val_if_fail (y >= 0, 0.0);
  g_return_val_if_fail (band < (self->num_bands), 0.0);
  g_return_val_if_fail (band >= 0, 0.0);

  GeglSampleModelClass *klass = GEGL_SAMPLE_MODEL_GET_CLASS (self);

  g_return_val_if_fail (klass->get_pixel_double, 0.0);

  return klass->get_sample_double (self, x, y, band, buffer);
}

void
gegl_sample_model_set_sample_double (const GeglSampleModel * self,
				     gint x,
				     gint y,
				     gint band,
				     gdouble sample, GeglBuffer * buffer)
{
  g_return_if_fail (GEGL_IS_SAMPLE_MODEL (self));
  g_return_if_fail (GEGL_IS_BUFFER (buffer));
  g_return_if_fail (x < (self->width));
  g_return_if_fail (x >= 0);
  g_return_if_fail (y < (self->height));
  g_return_if_fail (y >= 0);
  g_return_if_fail (band < (self->num_bands));
  g_return_if_fail (band >= 0);

  GeglSampleModelClass *klass = GEGL_SAMPLE_MODEL_GET_CLASS (self);

  g_return_if_fail (klass->get_pixel_double);

  klass->set_sample_double (self, x, y, band, sample, buffer);
}

static GeglNormalizer *
get_normalizer (const GeglSampleModel * self, gint band)
{
  if (self->normalizers == NULL)
    {
      return NULL;
    }
  GeglNormalizer *nor;
  if (self->normalizers->len >= self->num_bands)
    {
      nor = g_array_index (self->normalizers, GeglNormalizer *, band);
    }
  else
    {
      nor = g_array_index (self->normalizers, GeglNormalizer *, 0);
    }
  return nor;
}

gdouble
gegl_sample_model_get_sample_normalized (const GeglSampleModel * self,
					 gint x,
					 gint y,
					 gint band, const GeglBuffer * buffer)
{
  g_return_val_if_fail (self, 0.0);
  GeglSampleModelClass *class = GEGL_SAMPLE_MODEL_GET_CLASS (self);
  return class->get_sample_normalized (self, x, y, band, buffer);
}

void
gegl_sample_model_set_sample_normalized (const GeglSampleModel * self,
					 gint x,
					 gint y,
					 gint band,
					 gdouble sample, GeglBuffer * buffer)
{
  g_return_if_fail (self != NULL);
  GeglSampleModelClass *class = GEGL_SAMPLE_MODEL_GET_CLASS (self);
  class->set_sample_normalized (self, x, y, band, sample, buffer);

}

gdouble *
gegl_sample_model_get_pixel_normalized (const GeglSampleModel * self,
					gint x,
					gint y,
					gdouble * nor_data,
					const GeglBuffer * buffer)
{
  g_return_val_if_fail (self != NULL, NULL);
  GeglSampleModelClass *class = GEGL_SAMPLE_MODEL_GET_CLASS (self);
  return class->get_pixel_normalized (self, x, y, nor_data, buffer);
}

void
gegl_sample_model_set_pixel_normalized (const GeglSampleModel * self,
					gint x,
					gint y,
					const gdouble * nor_data,
					GeglBuffer * buffer)
{
  g_return_if_fail (self != NULL);
  GeglSampleModelClass *class = GEGL_SAMPLE_MODEL_GET_CLASS (self);
  return class->set_pixel_normalized (self, x, y, nor_data, buffer);
}

GeglBuffer *
gegl_sample_model_create_buffer (const GeglSampleModel * self,
				 TransferType type)
{

  g_return_val_if_fail (GEGL_IS_SAMPLE_MODEL (self), NULL);

  GeglSampleModelClass *class = GEGL_SAMPLE_MODEL_GET_CLASS (self);

  g_return_val_if_fail (class->create_buffer != NULL, NULL);

  GeglBuffer *new_buffer = class->create_buffer (self, type);

  g_return_val_if_fail (new_buffer != NULL, NULL);

  return new_buffer;
}

gboolean
gegl_sample_model_check_buffer (const GeglSampleModel * self,
				const GeglBuffer * buffer)
{
  g_return_val_if_fail (GEGL_IS_SAMPLE_MODEL (self), FALSE);
  GeglSampleModelClass *class = GEGL_SAMPLE_MODEL_GET_CLASS (self);
  g_return_val_if_fail (class->check_buffer != NULL, FALSE);

  return class->check_buffer (self, buffer);
}
