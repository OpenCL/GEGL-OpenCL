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

#ifndef GEGL_COMPONENT_SAMPLE_MODEL_H
#define GEGL_COMPONENT_SAMPLE_MODEL_H

#include "gegl-sample-model.h"

#define GEGL_TYPE_COMPONENT_SAMPLE_MODEL               (gegl_component_sample_model_get_type ())
#define GEGL_COMPONENT_SAMPLE_MODEL(obj)               (G_TYPE_CHECK_INSTANCE_CAST ((obj), GEGL_TYPE_COMPONENT_SAMPLE_MODEL, GeglComponentSampleModel))
#define GEGL_COMPONENT_SAMPLE_MODEL_CLASS(klass)       (G_TYPE_CHECK_CLASS_CAST ((klass),  GEGL_TYPE_COMPONENT_SAMPLE_MODEL, GeglComponentSampleModelClass))
#define GEGL_IS_COMPONENT_SAMPLE_MODEL(obj)            (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GEGL_TYPE_COMPONENT_SAMPLE_MODEL))
#define GEGL_IS_COMPONENT_SAMPLE_MODEL_CLASS(klass)    (G_TYPE_CHECK_CLASS_TYPE ((klass),  GEGL_TYPE_COMPONENT_SAMPLE_MODEL))
#define GEGL_COMPONENT_SAMPLE_MODEL_GET_CLASS(obj)     (G_TYPE_INSTANCE_GET_CLASS ((obj),  GEGL_TYPE_COMPONENT_SAMPLE_MODEL, GeglComponentSampleModelClass))

GType gegl_component_sample_model_get_type(void);

typedef struct _GeglComponentSampleModel GeglComponentSampleModel;
struct _GeglComponentSampleModel
{
  GeglSampleModel parent;
  /* private */
  gint pixel_stride;
  gint scanline_stride;
  GArray* bank_offsets;
  GArray* band_indicies;
  
  
};

typedef struct _GeglComponentSampleModelClass GeglComponentSampleModelClass;
struct _GeglComponentSampleModelClass
{
  GeglSampleModelClass parent_class;
};

gint gegl_component_sample_model_get_pixel_stride(const GeglComponentSampleModel* self);
gint gegl_component_sample_model_get_scanline_stride(const GeglComponentSampleModel* self);
gint gegl_component_sample_model_get_bank_offset(const GeglComponentSampleModel* self, gint bank);
gint gegl_component_sample_model_get_band_index(const GeglComponentSampleModel* self, gint band);


#endif


