/*
 *   This file is part of GEGL.
 *
 *    GEGL is free software; you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License as published by
 *    the Free Software Foundation; either version 2 of the License, or
 *    (at your option) any later version.
 *
 *    Foobar is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with Foobar; if not, write to the Free Software
 *    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 *  Copyright 2003 Daniel S. Rogers
 *
 */
 
#include "gegl-component-sample-model.h"

#include "glib.h"

/* overridden fuctions */
static void class_init(gpointer g_class,
                       gpointer class_data);
static void instance_init(GTypeInstance *instance,
                          gpointer g_class);
static GObject* constructor(GType type,
                            guint n_construct_properties,
                            GObjectConstructParam *construct_properties);
static void finalize(GObject *object);
static void get_property(GObject *object,
                         guint property_id,
                         GValue *value,
                         GParamSpec *pspec);
static void set_property(GObject *object,
                         guint property_id,
                         const GValue *value,
                         GParamSpec *pspec);
static gdouble get_sample_double(const GeglSampleModel* self,
				 gint x, 
				 gint y, 
				 gint band, 
				 const GeglBuffer* buffer);
static void set_sample_double(const GeglSampleModel* self,
			      gint x, 
			      gint y, 
			      gint band,
			      gdouble sample, 
			      GeglBuffer* buffer);
static GeglBuffer* create_buffer(const GeglSampleModel* self,
				 TransferType type);
static gboolean check_buffer(const GeglSampleModel* self,
			     const GeglBuffer* buffer);

/* private functions */

static GArray* g_array_copy_gint(const GArray* src);
inline static gint g_array_max_gint(GArray* array);


static gpointer parent_class;

enum {
  PROP_0,
  PROP_PIXEL_STRIDE,
  PROP_SCANLINE_STRIDE,
  PROP_BANK_OFFSETS,
  PROP_BAND_INDICIES,
  PROP_LAST
};

GType
gegl_component_sample_model_get_type (void)
{
  static GType type=0;
  if (!type)
    {
      static const GTypeInfo typeInfo =
	{
	  /* interface types, classed types, instantiated types */
	  sizeof(GeglComponentSampleModelClass),
	  NULL, //base_init
	  NULL, //base_finalize
	  
	  /* classed types, instantiated types */
	  class_init, //class_init
	  NULL, //class_finalize
	  NULL, //class_data
	  
	  /* instantiated types */
	  sizeof(GeglComponentSampleModel),
	  0, //n_preallocs
	  instance_init, //instance_init
	  
	  /* value handling */
	  NULL //value_table
	};
      
      type = g_type_register_static (GEGL_TYPE_SAMPLE_MODEL ,
				     "GeglComponentSampleModel",
				     &typeInfo,
				     0);
    }
  return type;
}

static void
class_init(gpointer g_class,
           gpointer class_data) {
  GeglSampleModelClass* sm_class=g_class;
  GObjectClass* gob_class=g_class;
  
  parent_class=g_type_class_peek_parent(sm_class);
  
  sm_class->get_sample_double=get_sample_double;
  sm_class->set_sample_double=set_sample_double;
  sm_class->create_buffer=create_buffer;
  sm_class->check_buffer=check_buffer;
    
  gob_class->get_property=get_property;
  gob_class->set_property=set_property;
  gob_class->constructor=constructor;
  gob_class->finalize=finalize;
  
  g_object_class_install_property(gob_class, PROP_PIXEL_STRIDE,
				  g_param_spec_int("pixel_stride",
						   "Pixel Stride",
						   "Distance in number of buffer elements between two samples for the same band and the same scanline.",
						   0,
						   G_MAXINT,
						   0,
						   G_PARAM_CONSTRUCT_ONLY|
						   G_PARAM_READWRITE));
  g_object_class_install_property(gob_class, PROP_SCANLINE_STRIDE,
				  g_param_spec_int("scanline_stride",
						   "Scanline Stride",
						   "Distance in number of buffer elements between a sample and the sample in the same column and the next scanline.",
						   0,
						   G_MAXINT,
						   0,
						   G_PARAM_CONSTRUCT_ONLY|
						   G_PARAM_READWRITE));
  g_object_class_install_property(gob_class, PROP_BANK_OFFSETS,
				  g_param_spec_pointer("bank_offsets",
						       "Bank Offsets",
						       "A GArray* of gints containing the offsets in buffer elements from the begining of a bank, to the first sample in the bank.",
						       G_PARAM_CONSTRUCT_ONLY|
						       G_PARAM_READWRITE));
  g_object_class_install_property(gob_class, PROP_BAND_INDICIES,
				  g_param_spec_pointer("band_indices",
						       "Band Indices",
						       "A GArray* of gints containing the indices of the bands that hold each bank.  There should be one for each band.",
						       G_PARAM_CONSTRUCT_ONLY|
						       G_PARAM_READWRITE));
  
}
static void
instance_init(GTypeInstance *instance,
              gpointer g_class) {
  GeglComponentSampleModel* csm=(GeglComponentSampleModel*)instance;
  
  csm->pixel_stride=0;
  csm->scanline_stride=0;
  csm->bank_offsets=NULL;
  csm->band_indicies=NULL;
}

static GObject*
constructor(GType type,
            guint n_construct_properties,
            GObjectConstructParam *construct_properties) {
  
  GObject* new_object=G_OBJECT_CLASS(parent_class)->constructor(type, n_construct_properties, construct_properties);
  GeglComponentSampleModel* csm=(GeglComponentSampleModel*)new_object;
  GeglSampleModel* sample_model=(GeglSampleModel*)new_object;
  
  if (csm->band_indicies) {
    gint num_indexed_bands=csm->band_indicies->len;
    if (num_indexed_bands!=sample_model->num_bands) {
      goto LABEL_failure;
    }
  }
  return new_object;

 LABEL_failure:
  g_object_unref(new_object);
  g_warning("GeglComponentSampleModel: Inconsistant argument types in the constructor");
  return NULL;
}

static void finalize(GObject *object) {
  GeglComponentSampleModel* csm=(GeglComponentSampleModel*)object;
  if (csm->bank_offsets!=NULL) {
    g_array_free(csm->bank_offsets,FALSE);
  }
  if (csm->band_indicies!=NULL) {
    g_array_free(csm->band_indicies,FALSE);
  }
}

static void
get_property(GObject *object,
	     guint property_id,
	     GValue *value,
	     GParamSpec *pspec){
  GeglComponentSampleModel* self=(GeglComponentSampleModel*)object;
    
    switch (property_id) {
    case PROP_PIXEL_STRIDE:
      g_value_set_int(value,gegl_component_sample_model_get_pixel_stride(self));
      break;
    case PROP_SCANLINE_STRIDE:
      g_value_set_int(value,gegl_component_sample_model_get_scanline_stride(self));
      break;
    case PROP_BANK_OFFSETS:
      g_value_set_pointer(value,self->bank_offsets);
      break;
    case PROP_BAND_INDICIES:
      g_value_set_pointer(value,self->band_indicies);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID(object,property_id,pspec);
      break;
    }
}

static void set_property(GObject *object,
                         guint property_id,
                         const GValue *value,
                         GParamSpec *pspec) {
  GeglComponentSampleModel* self=GEGL_COMPONENT_SAMPLE_MODEL(object);
  
  switch (property_id) {
  case PROP_PIXEL_STRIDE:
    self->pixel_stride=g_value_get_int(value);
    break;
  case PROP_SCANLINE_STRIDE:
    self->scanline_stride=g_value_get_int(value);
    break;
  case PROP_BANK_OFFSETS:
    self->bank_offsets=g_array_copy_gint((GArray*)g_value_get_pointer(value));
    break;
  case PROP_BAND_INDICIES:
    self->band_indicies=g_array_copy_gint((GArray*)g_value_get_pointer(value));
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID(object,property_id,pspec);
    break;
  }
}


static gdouble
get_sample_double(const GeglSampleModel* sample_model,
                  gint x, 
                  gint y, 
                  gint band, 
                  const GeglBuffer* buffer) {
  GeglComponentSampleModel* self=(GeglComponentSampleModel*)sample_model;
  g_return_val_if_fail(check_buffer(sample_model,buffer),0.0);
  gint bank=0;
  gint index=0;
  gint offset=0;

  bank=gegl_component_sample_model_get_band_index(self,band);
  offset=gegl_component_sample_model_get_bank_offset(self,bank);

  index=offset+y*(self->scanline_stride)+x*(self->pixel_stride);
  return gegl_buffer_get_element_double(buffer,bank,index);
  
}

static void
set_sample_double(const GeglSampleModel* sample_model,
                  gint x, 
                  gint y, 
                  gint band,
                  gdouble sample, 
                  GeglBuffer* buffer) {
  GeglComponentSampleModel* self=(GeglComponentSampleModel*)sample_model;
  g_return_if_fail(check_buffer(sample_model,buffer));
  
  gint bank=0;
  gint index=0;
  gint offset=0;

  bank=gegl_component_sample_model_get_band_index(self,band);
  offset=gegl_component_sample_model_get_bank_offset(self,bank);

  index=offset+y*(self->scanline_stride)+x*(self->pixel_stride);
  gegl_buffer_set_element_double(buffer,bank,index,sample);
}

static GArray*
g_array_copy_gint(const GArray* src) {
  if (src == NULL) {
    return NULL;
  }

  GArray* dst=g_array_new(FALSE,FALSE,sizeof(gint));
  gint i;
  for (i=0;i<src->len;i++) {
    g_array_append_val(dst,g_array_index(src,gint,i));
  }
  return dst;
}


static GeglBuffer*
create_buffer(const GeglSampleModel* sample_model,
              TransferType type) {
  GeglComponentSampleModel* self=GEGL_COMPONENT_SAMPLE_MODEL(sample_model);
  
  gint num_banks=0;
  if (self->band_indicies) {
    num_banks=g_array_max_gint(self->band_indicies) +1;
  } else {
    num_banks=1;
  }

  gint max_offset=0;
  if (self->bank_offsets) {
    max_offset=g_array_max_gint(self->band_indicies);
  } else {
    max_offset=0;
  }
  gint elements_per_bank=
    max_offset+
    (sample_model->width)*(self->pixel_stride)+
    (sample_model->height)*(self->scanline_stride);
  
  return gegl_buffer_create(type,"num_banks",num_banks,"elements_per_bank",elements_per_bank);
}

static gboolean
check_buffer(const GeglSampleModel* sample_model,
	     const GeglBuffer* buffer) {
  GeglComponentSampleModel* self=GEGL_COMPONENT_SAMPLE_MODEL(sample_model);
  
  gint max_bank;
  gint max_offset;
  if (self->band_indicies) {
    max_bank=g_array_max_gint(self->band_indicies);
  } else {
    max_bank=0;
  }
  if (self->bank_offsets) {
    max_offset=g_array_max_gint(self->bank_offsets);
  } else {
    max_offset=0;
  }

  if ((max_offset+(sample_model->width)*
       (self->pixel_stride)+
       (sample_model->height)*
       (self->scanline_stride))
      >gegl_buffer_get_elements_per_bank(buffer)) {
    return FALSE;
  }
  return TRUE;
}
 
inline static gint
g_array_max_gint(GArray* array) {
  gint i=0;
  gint max=g_array_index(array,gint,0);
  for (i=0;i<array->len;i++) {
    max= (g_array_index(array,gint,i)<max) ? max: g_array_index(array,gint,i);
  }
  return max;
}

gint
gegl_component_sample_model_get_pixel_stride(const GeglComponentSampleModel* self){
    g_return_val_if_fail(GEGL_IS_COMPONENT_SAMPLE_MODEL(self),0);
    
    return self->pixel_stride;
}

gint
gegl_component_sample_model_get_scanline_stride(const GeglComponentSampleModel* self) {
    g_return_val_if_fail(GEGL_IS_COMPONENT_SAMPLE_MODEL(self),0);
    return self->scanline_stride;
}


gint
gegl_component_sample_model_get_bank_offset(const GeglComponentSampleModel* self, gint bank) {
    g_return_val_if_fail(GEGL_IS_COMPONENT_SAMPLE_MODEL(self),0);
    
    if (self->bank_offsets == NULL) {
      return 0;
    }
    
    g_return_val_if_fail(bank < self->bank_offsets->len,0);
    return g_array_index(self->bank_offsets,gint,bank);
}

gint
gegl_component_sample_model_get_band_index(const GeglComponentSampleModel* self, gint band) {
    g_return_val_if_fail(GEGL_IS_COMPONENT_SAMPLE_MODEL(self),0);
    if (self->band_indicies == NULL ) {
      return 0;
    }

    g_return_val_if_fail(band < self->band_indicies->len,0);
    return g_array_index(self->band_indicies,gint,band);
}
