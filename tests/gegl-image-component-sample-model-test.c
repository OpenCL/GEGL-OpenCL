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

#include "image/gegl-component-sample-model.h"
#include "image/gegl-buffer-double.h"
#include "image/gegl-normalizer-mult.h"

#include <glib.h>

#include "ctest.h"
#include "csuite.h"
#include <string.h>

static void
test_g_object_new(Test *test) {
  GeglComponentSampleModel* csm=g_object_new(GEGL_TYPE_COMPONENT_SAMPLE_MODEL,
					     "width",64,
					     "height",64,
					     "num_bands",3,
					     "pixel_stride",3,
					     "scanline_stride",196,
					     NULL);
  ct_test(test,csm!=NULL);
  ct_test(test, GEGL_IS_COMPONENT_SAMPLE_MODEL(csm));
  ct_test(test, g_type_parent(GEGL_TYPE_COMPONENT_SAMPLE_MODEL) == GEGL_TYPE_SAMPLE_MODEL);
  ct_test(test, !strcmp("GeglComponentSampleModel", g_type_name(GEGL_TYPE_COMPONENT_SAMPLE_MODEL)));
  
  g_object_unref(csm);
}


static void
test_properties(Test* test) {
  GArray* bank_offsets=g_array_new(FALSE,FALSE,sizeof(gint));
  const gint offset=3;
  g_array_append_val(bank_offsets,offset);
  GArray* band_indices=g_array_new(FALSE,FALSE,sizeof(gint));
  const gint band_indices_data[]={0,1,2};
  g_array_append_vals(band_indices,band_indices_data,3);

  GeglComponentSampleModel* csm=g_object_new(GEGL_TYPE_COMPONENT_SAMPLE_MODEL,
					     "width",64,
					     "height",128,
					     "num_bands",3,
					     "pixel_stride",3,
					     "scanline_stride",64*3,
					     "bank_offsets",bank_offsets,
					     "band_indices",band_indices,
					     NULL);
  g_array_free(bank_offsets,FALSE);
  g_array_free(band_indices,FALSE);

  gint width;
  gint height;
  gint num_bands;
  gint pixel_stride;
  gint scanline_stride;
  
  GObject* object=G_OBJECT(csm);
  g_object_get(object,"width",&width,"height",&height,"num_bands",&num_bands,
	       "pixel_stride",&pixel_stride,"scanline_stride",
	       &scanline_stride,"bank_offsets",&bank_offsets,"band_indices",
	       &band_indices);
  ct_test(test,width==64);
  ct_test(test,height==128);
  ct_test(test,pixel_stride == 3);
  ct_test(test,scanline_stride==(width*3));
  ct_test(test,bank_offsets->len==1);
  ct_test(test,g_array_index(bank_offsets,gint,0) == 3);
  ct_test(test,band_indices->len==3);

  g_object_unref(object);
}



static void
test_pixel_interleaving(Test* test) {
  gint width=64;
  gint height=128;
  gint num_bands=3;
  GeglBufferDouble* buffer_double=g_object_new(GEGL_TYPE_BUFFER_DOUBLE,
					       "num_banks", 1,
					       "elements_per_bank", width*height*num_bands,
					       NULL);
  GeglBuffer* buffer=GEGL_BUFFER(buffer_double);
  GeglComponentSampleModel* csm=g_object_new(GEGL_TYPE_COMPONENT_SAMPLE_MODEL,
					     "num_bands",num_bands,
					     "width",width,
					     "height",height,
					     "pixel_stride",num_bands,
					     "scanline_stride",num_bands*width,
					     NULL);
  GeglSampleModel* sample_model=GEGL_SAMPLE_MODEL(csm);
  
  ct_test(test,gegl_sample_model_check_buffer(sample_model,buffer)==TRUE);
  
  GeglBuffer* new_buffer=gegl_sample_model_create_buffer(sample_model,TYPE_DOUBLE);
  
  ct_test(test,gegl_sample_model_check_buffer(sample_model,new_buffer)==TRUE);
  
  gint num_banks1,num_banks2,num_elements1,num_elements2;

  g_object_get(buffer,"num_banks",&num_banks1,"elements_per_bank",&num_elements1,NULL);
  g_object_get(buffer,"num_banks",&num_banks2,"elements_per_bank",&num_elements2,NULL);

  ct_test(test,num_banks1 == num_banks2);
  ct_test(test,num_elements1 == num_elements2);

  gint x,y,band;
  for (y=0;y<height;y++) {
    for (x=0;x<width;x++) {
      for (band=0;band<num_bands;band++) {
	gegl_sample_model_set_sample_double(sample_model,x,y,band,x+y+band,buffer);
      }
    }
  }
  gboolean all_OK=TRUE;
  for (y=0;y<height;y++) {
    for (x=0;x<width;x++) {
      for (band=0;band<num_bands;band++) {
	if (gegl_sample_model_get_sample_double(sample_model,x,y,band,buffer) != (x+y+band)) {
	  all_OK=FALSE;
	  break;
	}
      }
      if (all_OK==FALSE) {
	break;
      }
    }
    if (all_OK==FALSE) {
      break;
    }
  }
  
  ct_test(test,all_OK==TRUE);
  
  gdouble** banks=(gdouble**)gegl_buffer_get_banks(buffer);
  gdouble* bank=banks[0];
  
  all_OK=TRUE;
  for (y=0;y<height;y++) {
    for (x=0;x<width;x++) {
      for (band=0;band<num_bands;band++) {
	//this breaks if num_bands != pixel_stride
	//and scanline_stride != width*num_bands
	if (bank[y*width*num_bands+x*num_bands+band] != (x+y+band)) {
	  all_OK=FALSE;
	}
      }
      if (all_OK==FALSE) {
	break;
      }
    }
    if (all_OK==FALSE) {
      break;
    }
  }

  ct_test(test,all_OK==TRUE);

  g_object_unref(new_buffer);
  g_object_unref(buffer_double);
  g_object_unref(csm);
}

static void
test_band_interleaving(Test* test) {
  gint width=64;
  gint height=128;
  gint num_bands=3;
  GeglBufferDouble* buffer_double=g_object_new(GEGL_TYPE_BUFFER_DOUBLE,
					       "num_banks", num_bands,
					       "elements_per_bank", width*height,
					       NULL);
  GeglBuffer* buffer=GEGL_BUFFER(buffer_double);
  
  GArray* band_indices=g_array_new(FALSE,FALSE,sizeof(gint));
  gint data[3]={0,1,2};
  g_array_append_vals(band_indices,data,3);
  GeglComponentSampleModel* csm=g_object_new(GEGL_TYPE_COMPONENT_SAMPLE_MODEL,
					     "num_bands",num_bands,
					     "width",width,
					     "height",height,
					     "pixel_stride",1,
					     "scanline_stride",width,
					     "band_indices", band_indices,
					     NULL);
  g_array_free(band_indices,FALSE);
  GeglSampleModel* sample_model=GEGL_SAMPLE_MODEL(csm);
  ct_test(test,gegl_sample_model_check_buffer(sample_model,buffer)==TRUE);
  
  GeglBuffer* new_buffer=gegl_sample_model_create_buffer(sample_model,TYPE_DOUBLE);
  
  ct_test(test,gegl_sample_model_check_buffer(sample_model,new_buffer)==TRUE);
  
  gint num_banks1,num_banks2,num_elements1,num_elements2;

  g_object_get(buffer,"num_banks",&num_banks1,"elements_per_bank",&num_elements1,NULL);
  g_object_get(buffer,"num_banks",&num_banks2,"elements_per_bank",&num_elements2,NULL);
  
  ct_test(test,num_banks1 == num_banks2);
  ct_test(test,num_elements1 == num_elements2);


  gint x,y,band;
    for (y=0;y<height;y++) {
    for (x=0;x<width;x++) {
      for (band=0;band<num_bands;band++) {
	gegl_sample_model_set_sample_double(sample_model,x,y,band,x+y+band,buffer);
      }
    }
  }
  gboolean all_OK=TRUE;
    for (y=0;y<height;y++) {
        for (x=0;x<width;x++) {
            for (band=0;band<num_bands;band++) {
	           if (gegl_sample_model_get_sample_double(sample_model,x,y,band,buffer) != (x+y+band)) {
	               all_OK=FALSE;
	               break;
	           }
            }
            if (all_OK==FALSE) {
	           break;
            }
        }
        if (all_OK==FALSE) {
            break;
        }
    }

  ct_test(test,all_OK==TRUE);
  
  gdouble** banks=(gdouble**)gegl_buffer_get_banks(buffer);

  all_OK=TRUE;
  
  //here is the actual band interleaved specific part

  for (y=0;y<height;y++) {
    for (x=0;x<width;x++) {
      for (band=0;band<num_bands;band++) {
	if (banks[band][y*width+x] != x+y+band) {
	  all_OK=FALSE;
	}
      }
      if (all_OK==FALSE) {
	break;
      }
    }
    if (all_OK==FALSE) {
      break;
    }
  }
  ct_test(test,all_OK==TRUE);
  
  g_object_unref(new_buffer);
  g_object_unref(buffer);
  g_object_unref(csm);
  return;
}

static void 
test_normalizers(Test* test)
{
    GArray* normalizers=g_array_new(FALSE,FALSE,sizeof(GeglNormalizer*));
    GeglNormalizer* nor0=g_object_new(GEGL_TYPE_NORMALIZER_MULT,
                                      "alpha",2.0,
                                      NULL);
    GeglNormalizer* nor1=g_object_new(GEGL_TYPE_NORMALIZER_MULT,
                                      "alpha",3.0,
                                      NULL);
    GeglNormalizer* nor2=g_object_new(GEGL_TYPE_NORMALIZER_MULT,
                                      "alpha",4.0,
                                      NULL);
    g_array_append_val(normalizers,nor0);
    g_array_append_val(normalizers,nor1);
    g_array_append_val(normalizers,nor2);


    gint width=64;
    gint height=128;
    gint num_bands=3;
    GeglComponentSampleModel* csm=
                              g_object_new(GEGL_TYPE_COMPONENT_SAMPLE_MODEL,
                                           "num_bands",num_bands,
                                           "width",width,
                                           "height",height,
                                           "pixel_stride",num_bands,
                                           "scanline_stride",num_bands*width,
                                           "normalizers",normalizers,
                                           NULL);
    GeglSampleModel* sample_model=GEGL_SAMPLE_MODEL(csm);
    GeglBufferDouble* buffer_double=(GeglBufferDouble*)gegl_sample_model_create_buffer(sample_model,TYPE_DOUBLE);
    GeglBuffer* buffer=GEGL_BUFFER(buffer_double);
    gint x,y,band;
    for (y=0;y<height;y++) {
        for (x=0;x<width;x++) {
            for (band=0;band<num_bands;band++) {
                gegl_sample_model_set_sample_double(sample_model,x,y,band,x+y+band,buffer);
            }
        }
    }
    gboolean all_OK=TRUE;
    gdouble sample,nor_sample;
    for (y=0;y<height;y++) {
        for (x=0;x<width;x++) {
            for (band=0;band<num_bands;band++) {
                sample=gegl_sample_model_get_sample_double(sample_model,x,y,band,buffer);
                nor_sample=gegl_sample_model_get_sample_normalized(sample_model,x,y,band,buffer);
                if (nor_sample != (sample * (band+2))) {
                    all_OK=FALSE;
                    break;
                }
            }
            if (all_OK==FALSE) {
            break;
        }
    }
    if (all_OK==FALSE) {
      break;
    }
  }

  ct_test(test,all_OK==TRUE);

  g_object_unref(nor0);
  g_object_unref(nor1);
  g_object_unref(nor2);
  g_object_unref(csm);
  g_object_unref(buffer_double);
}

Test *
create_component_sample_model_test()
{
  Test* t = ct_create("GeglComponentSampleModelTest");
  
  g_assert(ct_addTestFun(t, test_g_object_new));
  g_assert(ct_addTestFun(t, test_properties));
  g_assert(ct_addTestFun(t, test_pixel_interleaving));
  g_assert(ct_addTestFun(t, test_band_interleaving));
  g_assert(ct_addTestFun(t, test_normalizers));

  return t; 
}

