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
 
#include "gegl-buffer-double.h"

static void instance_init (GTypeInstance *instance, gpointer g_class);
static void class_init (gpointer g_class, gpointer class_data);
static GObject* constructor(GType type,
                            guint n_construct_properties,
                            GObjectConstructParam *construct_properties);
static gdouble get_element_double (GeglBuffer* self, gint bank,gint index);
static void set_element_double(GeglBuffer* self, gint bank,gint index, gdouble elem);

static gpointer parent_class;

GType
gegl_buffer_double_get_type (void)
{
	static GType type=0;
	if (!type)
	{
		static const GTypeInfo typeInfo =
		{
			/* interface types, classed types, instantiated types */
			sizeof(GeglBufferDoubleClass),
			NULL,
			NULL,
			
			/* classed types, instantiated types */
			class_init,
			NULL,
			NULL,
			
			/* instantiated types */
			sizeof(GeglBufferDouble),
			10,
			instance_init,
			
			/* value handling */ 
			NULL
		};
		
		type = g_type_register_static (GEGL_TYPE_BUFFER ,
														"GeglBufferDouble",
														&typeInfo,
													    0);
	}
	return type;
}
static void class_init (gpointer g_class, gpointer class_data) {
    GeglBufferDoubleClass* klass=g_class;
    GeglBufferClass* buffer_class=GEGL_BUFFER_CLASS(klass);
    GObjectClass* object_class=G_OBJECT_CLASS(klass);
    
    parent_class=g_type_class_peek_parent(g_class);
    
    object_class->constructor=constructor;
    buffer_class->get_element_double=get_element_double;
    buffer_class->set_element_double=set_element_double;
    
}

static GObject* constructor(GType type,
                            guint n_construct_properties,
                            GObjectConstructParam *construct_properties) {
    return G_OBJECT_CLASS(parent_class)->constructor(type,n_construct_properties,construct_properties);
}

static void set_element_double(GeglBuffer* self, gint bank,gint index,gdouble elem) {
    
    g_return_if_fail(GEGL_IS_BUFFER_DOUBLE(self));
    g_return_if_fail(self->num_banks > bank);
    g_return_if_fail(self->elements_per_bank > index);
    
    ((gdouble**)self->banks)[bank][index]=elem;
}

static gdouble get_element_double(GeglBuffer* self, gint bank, gint index) {
    
    g_return_val_if_fail(GEGL_IS_BUFFER_DOUBLE(self),0.0);
    g_return_val_if_fail(self->num_banks > bank,0.0);
    g_return_val_if_fail(self->elements_per_bank > index,0.0);
    
    return ((gdouble**)self->banks)[bank][index];
}

static void instance_init(GTypeInstance* instance, gpointer g_class) {
    GeglBufferDouble* self=(GeglBufferDouble*)instance;
    GeglBuffer* parent=GEGL_BUFFER(self);
    parent->bytes_per_element=sizeof(gdouble);
}
