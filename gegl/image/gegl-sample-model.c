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

#include <glib-object.h>
#include "gegl-sample-model.h"

static gpointer parent_class;

enum {
  PROP_0,
  PROP_WIDTH,
  PROP_HEIGHT,
  PROP_NUM_BANDS,
  PROP_LAST
};

static void instance_init(GTypeInstance *instance,
                          gpointer g_class);
static void class_init(gpointer g_class,
                       gpointer class_data);
static void get_property(GObject *object,
                         guint property_id,
                         GValue *value,
                         GParamSpec *pspec);
static void set_property(GObject *object,
                         guint property_id,
                         const GValue *value,
                         GParamSpec *pspec);
static gdouble*
get_pixel_double_default(const GeglSampleModel* self,
                         gint x,
                         gint y,
                         gdouble* dArray,
                         const GeglBuffer* buffer);
static void
set_pixel_double_default(const GeglSampleModel* self,
                         gint x,
                         gint y,
                         const gdouble* dArray,
                         GeglBuffer* buffer);


GType
gegl_sample_model_get_type (void)
{
	static GType type=0;
	if (!type)
	{
		static const GTypeInfo typeInfo =
		{
			/* interface types, classed types, instantiated types */
			sizeof(GeglSampleModelClass),
			NULL, //base_init
			NULL, //base_finalize
			
			/* classed types, instantiated types */
			class_init, //class_init
			NULL, //class_finalize
			NULL, //class_data
			
			/* instantiated types */
			sizeof(GeglSampleModel),
			0, //n_preallocs
			instance_init, //instance_init
			
			/* value handling */
			NULL //value_table
		};
		
		type = g_type_register_static (GEGL_TYPE_OBJECT ,
														"GeglSampleModel",
														&typeInfo,
														G_TYPE_FLAG_ABSTRACT);
	}
	return type;
}

static void
instance_init(GTypeInstance *instance, gpointer g_class) {
    GeglSampleModel* sample_model=(GeglSampleModel*)instance;
    
    sample_model->width=0;
    sample_model->height=0;
    sample_model->num_bands=0;
}

static void
class_init(gpointer g_class, gpointer class_data) {
    GeglSampleModelClass* sm_class=(GeglSampleModelClass*)g_class;
    GObjectClass* object_class=(GObjectClass*)g_class;
    
    sm_class->get_pixel_double=get_pixel_double_default;
    sm_class->set_pixel_double=set_pixel_double_default;
    
    object_class->get_property=get_property;
    object_class->set_property=set_property;
        
    g_object_class_install_property(object_class, PROP_WIDTH,
                                    g_param_spec_int("width",
                                                     "Width",
                                                     "Width of the buffer described by this sample model",
                                                     0,
                                                     G_MAXINT,
                                                     0,
                                                     G_PARAM_CONSTRUCT_ONLY|
                                                     G_PARAM_READWRITE));
    g_object_class_install_property(object_class, PROP_HEIGHT,
                                    g_param_spec_int("height",
                                                     "Height",
                                                     "Height of the buffer described by this sample model",
                                                     0,
                                                     G_MAXINT,
                                                     0,
                                                     G_PARAM_CONSTRUCT_ONLY|
                                                     G_PARAM_READWRITE));
                                    
    g_object_class_install_property(object_class, PROP_NUM_BANDS,
                                    g_param_spec_int("num_bands",
                                                     "Number of Bands",
                                                     "Number of Bands in the buffer described by this sample model",
                                                     0,
                                                     G_MAXINT,
                                                     0,
                                                     G_PARAM_CONSTRUCT_ONLY|
                                                     G_PARAM_READWRITE));
    
    parent_class=g_type_class_peek_parent(g_class);
    
}

static void
get_property(GObject *gobject, guint property_id, GValue *value, GParamSpec *pspec) {
    GeglSampleModel* self=(GeglSampleModel*)gobject;
    switch (property_id) {
        case PROP_WIDTH:
            g_value_set_int(value, gegl_sample_model_get_width(self));
            return;
        case PROP_HEIGHT:
            g_value_set_int(value, gegl_sample_model_get_height(self));
            return;
        case PROP_NUM_BANDS:
            g_value_set_int(value, gegl_sample_model_get_num_bands(self));
            return;
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID (gobject, property_id, pspec);
            break;
    }

}

static void
set_property(GObject *object, guint property_id, const GValue *value, GParamSpec *pspec) {
    GeglSampleModel* self=(GeglSampleModel*)object;
    switch (property_id) {
        case PROP_WIDTH:
            self->width=g_value_get_int(value);
            return;
        case PROP_HEIGHT:
            self->height=g_value_get_int(value);
            return;
        case PROP_NUM_BANDS:
            self->num_bands=g_value_get_int(value);
            return;
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
            break;
    }
}

gint
gegl_sample_model_get_num_bands(const GeglSampleModel* self) {
    g_return_val_if_fail(GEGL_IS_SAMPLE_MODEL(self),0);
    
    return self->num_bands;
}

gint gegl_sample_model_get_width(const GeglSampleModel* self) {
    g_return_val_if_fail(GEGL_IS_SAMPLE_MODEL(self),0);
    
    return self->width;
}

gint gegl_sample_model_get_height(const GeglSampleModel* self) {
    g_return_val_if_fail(GEGL_IS_SAMPLE_MODEL(self),0);
    
    return self->height;
}

gdouble* gegl_sample_model_get_pixel_double(const GeglSampleModel* self,
                                            gint x,
                                            gint y,
                                            gdouble* dArray,
                                            const GeglBuffer* buffer) {
    g_return_val_if_fail(GEGL_IS_SAMPLE_MODEL(self),NULL);
    g_return_val_if_fail(dArray != NULL, NULL);
    g_return_val_if_fail(GEGL_IS_BUFFER(buffer),NULL);
    g_return_val_if_fail(x < (self->width),NULL);
    g_return_val_if_fail(x >= 0, NULL);
    g_return_val_if_fail(y < (self->height),NULL);
    g_return_val_if_fail(y >= 0, NULL);
    
    GeglSampleModelClass* klass=GEGL_SAMPLE_MODEL_GET_CLASS(self);
    
    g_return_val_if_fail(klass->get_pixel_double,NULL);
    
    return klass->get_pixel_double(self,x,y,dArray,buffer);
}

static gdouble*
get_pixel_double_default(const GeglSampleModel* self,
                         gint x,
                         gint y,
                         gdouble* dArray,
                         const GeglBuffer* buffer) {
    int i=0;
    gdouble* dArray_pos=dArray;
    for (i=0;i<self->num_bands;i++) {
        *dArray_pos = gegl_sample_model_get_sample_double(self,x,y,i,buffer);
        dArray_pos++;
    }
    return dArray;
}

void
gegl_sample_model_set_pixel_double(const GeglSampleModel* self,
                                   gint x,
                                   gint y,
                                   const gdouble* dArray,
                                   GeglBuffer* buffer) {
    g_return_if_fail(GEGL_IS_SAMPLE_MODEL(self));
    g_return_if_fail(dArray != NULL);
    g_return_if_fail(GEGL_IS_BUFFER(buffer));
    g_return_if_fail(x < (self->width));
    g_return_if_fail(x >= 0);
    g_return_if_fail(y < (self->height));
    g_return_if_fail(y >= 0);

    GeglSampleModelClass* klass=GEGL_SAMPLE_MODEL_GET_CLASS(self);
        
    g_return_if_fail(klass->get_pixel_double);
    
    klass->set_pixel_double(self,x,y,dArray,buffer);
}

static void
set_pixel_double_default(const GeglSampleModel* self,
                         gint x,
                         gint y,
                         const gdouble* dArray,
                         GeglBuffer* buffer) {
    //already bounds check the arguments in the non-static accessor.
    int i=0;
    for (i=0;i<self->num_bands;i++) {
        gegl_sample_model_set_sample_double(self,x,y,i,*dArray,buffer);
        dArray++;
    }
    return;
}

gdouble
gegl_sample_model_get_sample_double(const GeglSampleModel* self,
                                    gint x,
                                    gint y,
                                    gint band,
                                    const GeglBuffer* buffer) {
    g_return_val_if_fail(GEGL_IS_SAMPLE_MODEL(self),0.0);
    g_return_val_if_fail(GEGL_IS_BUFFER(buffer),0.0);
    g_return_val_if_fail(x < (self->width),0.0);
    g_return_val_if_fail(x >= 0, 0.0);
    g_return_val_if_fail(y < (self->height),0.0);
    g_return_val_if_fail(y >= 0, 0.0);
    g_return_val_if_fail(band < (self->num_bands) , 0.0);
    g_return_val_if_fail(band >= 0, 0.0);

    GeglSampleModelClass* klass=GEGL_SAMPLE_MODEL_GET_CLASS(self);
    
    g_return_val_if_fail(klass->get_pixel_double,0.0);
    
    return klass->get_sample_double(self,x,y,band,buffer);
}
void
gegl_sample_model_set_sample_double(const GeglSampleModel* self,
                                    gint x,
                                    gint y,
                                    gint band,
                                    gdouble sample,
                                    GeglBuffer* buffer) {
    g_return_if_fail(GEGL_IS_SAMPLE_MODEL(self));
    g_return_if_fail(GEGL_IS_BUFFER(buffer));
    g_return_if_fail(x < (self->width));
    g_return_if_fail(x >= 0);
    g_return_if_fail(y < (self->height));
    g_return_if_fail(y >= 0);
    g_return_if_fail(band < (self->num_bands));
    g_return_if_fail(band >= 0);

    GeglSampleModelClass* klass=GEGL_SAMPLE_MODEL_GET_CLASS(self);
    
    g_return_if_fail(klass->get_pixel_double);

    klass->set_sample_double(self,x,y,band,sample,buffer);
}

GeglBuffer*
gegl_sample_model_create_buffer(const GeglSampleModel* self,
                                TransferType type) {

    g_return_val_if_fail(GEGL_IS_SAMPLE_MODEL(self),NULL);
    
    GeglSampleModelClass* class=GEGL_SAMPLE_MODEL_GET_CLASS(self);
    
    g_return_val_if_fail(class->create_buffer != NULL,NULL);
    
    GeglBuffer* new_buffer=class->create_buffer(self,type);
    
    g_return_val_if_fail(new_buffer!=NULL,NULL);
    
    return new_buffer;
}
