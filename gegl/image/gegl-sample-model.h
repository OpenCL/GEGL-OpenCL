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

#ifndef GEGL_SAMPLE_MODEL_H
#define GEGL_SAMPLE_MODEL_H

#include "gegl-object.h"
#include "gegl-buffer.h"

#define GEGL_TYPE_SAMPLE_MODEL               (gegl_sample_model_get_type ())
#define GEGL_SAMPLE_MODEL(obj)               (G_TYPE_CHECK_INSTANCE_CAST ((obj), GEGL_TYPE_SAMPLE_MODEL, GeglSampleModel))
#define GEGL_SAMPLE_MODEL_CLASS(klass)       (G_TYPE_CHECK_CLASS_CAST ((klass),  GEGL_TYPE_SAMPLE_MODEL, GeglSampleModelClass))
#define GEGL_IS_SAMPLE_MODEL(obj)            (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GEGL_TYPE_SAMPLE_MODEL))
#define GEGL_IS_SAMPLE_MODEL_CLASS(klass)    (G_TYPE_CHECK_CLASS_TYPE ((klass),  GEGL_TYPE_SAMPLE_MODEL))
#define GEGL_SAMPLE_MODEL_GET_CLASS(obj)     (G_TYPE_INSTANCE_GET_CLASS ((obj),  GEGL_TYPE_SAMPLE_MODEL, GeglSampleModelClass))

GType gegl_sample_model_get_type(void);

typedef struct _GeglSampleModel GeglSampleModel;
struct _GeglSampleModel
{
    GeglObject parent;
    /* private */
    gint width;
    gint height;
    gint num_bands;
    GArray* normalizers;
};

typedef struct _GeglSampleModelClass GeglSampleModelClass;
struct _GeglSampleModelClass
{
  //these virutal functions have a default implementation
  GeglObjectClass parent_class;
  gdouble* (*get_pixel_double)(const GeglSampleModel* self,
			       gint x, 
			       gint y, 
			       gdouble* dArray, 
			       const GeglBuffer* buffer);
  void (*set_pixel_double)(const GeglSampleModel* self,
			   gint x, 
			   gint y, 
			   const gdouble* dArray, 
			   GeglBuffer* buffer);
  gdouble (*get_sample_normalized)(const GeglSampleModel * self,
                                   gint x,
                                   gint y,
                                   gint band,
                                   const GeglBuffer* buffer);
  void (*set_sample_normalized)(const GeglSampleModel * self,
                                gint x,
                                gint y,
                                gint band,
                                gdouble sample,
                                GeglBuffer* buffer);
  gdouble* (*get_pixel_normalized)(const GeglSampleModel * self,
                                   gint x,
                                   gint y,
                                   gdouble* d_array,
                                   const GeglBuffer* buffer);
  void (*set_pixel_normalized)(const GeglSampleModel* self,
                               gint x,
                               gint y,
                               const gdouble* d_array,
                               GeglBuffer* buffer);
  
  //pure virtual functions (i.e. you _must_ provide an implementation)
  gdouble (*get_sample_double)(const GeglSampleModel* self,
			       gint x, 
			       gint y, 
			       gint band, 
			       const GeglBuffer* buffer);
  void (*set_sample_double)(const GeglSampleModel* self,
			    gint x, 
			    gint y, 
			    gint band,
			    gdouble sample, 
			    GeglBuffer* buffer);

  GeglBuffer* (*create_buffer)(const GeglSampleModel* self,
			       TransferType type);
  gboolean (*check_buffer)(const GeglSampleModel* self,
			   const GeglBuffer* buffer);
};

gint gegl_sample_model_get_num_bands(const GeglSampleModel* self);
gint gegl_sample_model_get_width(const GeglSampleModel* self);
gint gegl_sample_model_get_height(const GeglSampleModel* self);
gdouble* gegl_sample_model_get_pixel_double(const GeglSampleModel* self,
                                            gint x,
                                            gint y,
                                            gdouble* dArray,
                                            const GeglBuffer* buffer);
void gegl_sample_model_set_pixel_double(const GeglSampleModel* self,
                                        gint x,
                                        gint y,
                                        const gdouble* dArray,
                                        GeglBuffer* buffer);
gdouble gegl_sample_model_get_sample_double(const GeglSampleModel* self,
                                            gint x,
                                            gint y,
                                            gint band,
                                            const GeglBuffer* buffer);
void gegl_sample_model_set_sample_double(const GeglSampleModel* self,
                                         gint x,
                                         gint y,
                                         gint band,
                                         gdouble sample,
                                         GeglBuffer* buffer);

gdouble gegl_sample_model_get_sample_normalized(const GeglSampleModel * self,
                                                gint x,
                                                gint y,
                                                gint band,
                                                const GeglBuffer* buffer);
void gegl_sample_model_set_sample_normalized(const GeglSampleModel * self,
                                             gint x,
                                             gint y,
                                             gint band,
                                             gdouble sample,
                                             GeglBuffer* buffer);
gdouble* gegl_sample_model_get_pixel_normalized(const GeglSampleModel * self,
                                                gint x,
                                                gint y,
                                                gdouble* d_array,
                                                const GeglBuffer* buffer);
void gegl_sample_model_set_pixel_normalized(const GeglSampleModel* self,
                                            gint x,
                                            gint y,
                                            const gdouble* d_array,
                                            GeglBuffer* buffer);
GeglBuffer* gegl_sample_model_create_buffer(const GeglSampleModel* self,
                                            TransferType type);
gboolean gegl_sample_model_check_buffer(const GeglSampleModel* self,
					const GeglBuffer* buffer);

#endif // GEGL_SAMPLE_MODEL_H
