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
 
#include "gegl-normalizer.h"

GType gegl_normalizer_get_type (void);
static void class_init(gpointer g_class,
                       gpointer class_data);

GType
gegl_normalizer_get_type (void)
{
	static GType type=0;
	if (!type)
	{
		static const GTypeInfo typeInfo =
		{
			/* interface types, classed types, instantiated types */
			sizeof(GeglNormalizerClass),
			NULL, //base_init
			NULL, //base_finalize
			
			/* classed types, instantiated types */
			class_init, //class_init
			NULL, //class_finalize
			NULL, //class_data
			
			/* instantiated types */
			sizeof(GeglNormalizer),
			0, //n_preallocs
			NULL, //instance_init
			
			/* value handling */
			NULL //value_table
		};
		
		type = g_type_register_static (GEGL_TYPE_OBJECT ,
										"GeglNormalizer",
										&typeInfo,
										G_TYPE_FLAG_ABSTRACT);
	}
	return type;
}

static void
class_init(gpointer g_class,
           gpointer class_data) {
    GeglNormalizerClass* class=GEGL_NORMALIZER_CLASS(g_class);
    class->normalize=NULL;
    class->unnormalize=NULL;
}

gdouble*
gegl_normalizer_normalize(const GeglNormalizer* self,
                          const gdouble* unnor_data,
                          gdouble* nor_data,
                          gint length,
                          gint stride)
{
    GeglNormalizerClass* class=GEGL_NORMALIZER_GET_CLASS(self);
    g_return_val_if_fail(GEGL_IS_NORMALIZER(self), NULL);
    g_return_val_if_fail(unnor_data!=NULL,NULL);
    g_return_val_if_fail(class->normalize!=NULL,NULL);
    
    if (nor_data == NULL) {
        nor_data=g_new(gdouble,length);
    }
    return class->normalize(self,unnor_data,nor_data,length,stride);
}

gdouble*
gegl_normalizer_unnormalize(const GeglNormalizer* self,
                            const gdouble* nor_data,
                            gdouble* unnor_data,
                            gint length,
                            gint stride)
{
    GeglNormalizerClass* class=GEGL_NORMALIZER_GET_CLASS(self);
    g_return_val_if_fail(GEGL_IS_NORMALIZER(self), NULL);
    g_return_val_if_fail(unnor_data!=NULL,NULL);
    g_return_val_if_fail(class->unnormalize!=NULL,NULL);
    
    if (nor_data == NULL) {
        nor_data=g_new(gdouble,length);
    }
    return class->unnormalize(self,nor_data,unnor_data,length,stride);
}
