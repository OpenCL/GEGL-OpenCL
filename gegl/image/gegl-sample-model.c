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
    //GeglSampleModelClass* sm_class=(GeglSampleModelClass*)g_class;
    GObjectClass* object_class=(GObjectClass*)g_class;
    
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
gegl_sample_model_get_num_bands(GeglSampleModel* self) {
    g_return_val_if_fail(GEGL_IS_SAMPLE_MODEL(self),0);
    
    return self->num_bands;
}

gint gegl_sample_model_get_width(GeglSampleModel* self) {
    g_return_val_if_fail(GEGL_IS_SAMPLE_MODEL(self),0);
    
    return self->width;
}

gint gegl_sample_model_get_height(GeglSampleModel* self) {
    g_return_val_if_fail(GEGL_IS_SAMPLE_MODEL(self),0);
    
    return self->height;
}

gdouble* gegl_sample_model_get_pixel_double(GeglSampleModel* self,
                                            gint x,
                                            gint y,
                                            gdouble* dArray,
                                            GeglBuffer* buffer) {
    g_return_val_if_fail(GEGL_IS_SAMPLE_MODEL(self),NULL);
    GeglSampleModelClass* klass=GEGL_SAMPLE_MODEL_GET_CLASS(self);
    
    g_return_val_if_fail(klass->get_pixel_double,NULL);
    
    return klass->get_pixel_double(self,x,y,dArray,buffer);
}
    
void gegl_sample_model_set_pixel_double(GeglSampleModel* self,
                                        gint x,
                                        gint y,
                                        gdouble* dArray,
                                        GeglBuffer* buffer) {
    g_return_if_fail(GEGL_IS_SAMPLE_MODEL(self));
    GeglSampleModelClass* klass=GEGL_SAMPLE_MODEL_GET_CLASS(self);
    
    g_return_if_fail(klass->get_pixel_double);
    
    klass->set_pixel_double(self,x,y,dArray,buffer);
}
gdouble
gegl_sample_model_get_sample_double(GeglSampleModel* self,
                                    gint x,
                                    gint y,
                                    gint band,
                                    GeglBuffer* buffer) {
    g_return_val_if_fail(GEGL_IS_SAMPLE_MODEL(self),0.0);
    GeglSampleModelClass* klass=GEGL_SAMPLE_MODEL_GET_CLASS(self);
    
    g_return_val_if_fail(klass->get_pixel_double,0.0);
    
    return klass->get_sample_double(self,x,y,band,buffer);
}
void
gegl_sample_model_set_sample_double(GeglSampleModel* self,
                                    gint x,
                                    gint y,
                                    gint band,
                                    gdouble sample,
                                    GeglBuffer* buffer) {
    g_return_if_fail(GEGL_IS_SAMPLE_MODEL(self));
    GeglSampleModelClass* klass=GEGL_SAMPLE_MODEL_GET_CLASS(self);
    
    g_return_if_fail(klass->get_pixel_double);

    klass->set_sample_double(self,x,y,band,sample,buffer);
}

GeglBuffer*
gegl_sample_model_create_buffer(GeglSampleModel* self,
                                TransferType type) {

    //TODO unimplemented
    return NULL;
}
