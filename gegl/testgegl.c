#include <gtk/gtk.h>
#include <gdk/gdkprivate.h>
#include <stdio.h>

#include "gegl-color.h"
#include "gegl-color-model-rgb-float.h"
#include "gegl-drawable.h"
#include "gegl-graphics-state.h"
#include "gegl-image-buffer.h"
#include "gegl-fill-op.h"
#include "gegl-print-op.h"
#include "gegl-graphics-state.h"
#include "gegl-utils.h"



int
main (int argc, char *argv[])
{  
  gtk_init (&argc, &argv);

#if 0      /*This uses the GeglDrawable class */ 
  {

	GeglColorModel *cm = GEGL_COLOR_MODEL(gegl_color_model_rgb_float_new(FALSE,FALSE));
	GeglDrawable *d = GEGL_DRAWABLE(gegl_drawable_new(cm,"myDrawable",5,5));
	GeglGraphicsState *state = gegl_drawable_get_graphics_state (d);
	GeglRect r;

	gegl_rect_set (&r, 0,0,5,5);
        gegl_color_set_constant (state->fg_color, COLOR_RED);
        gegl_drawable_fill(d, &r); 

	gegl_rect_set (&r, 1,1,4,4);
        gegl_color_set_constant (state->fg_color, COLOR_GREEN);
        gegl_drawable_fill(d, &r); 


        /* Print out the image values using the 
	   print operator 
        */
        {
          GeglOp *op;
	  GeglRect r;
	  gegl_rect_set (&r, 0,0,5,5);
	  op = GEGL_OP (gegl_print_op_new (gegl_drawable_get_image_buffer(d), &r));
          gegl_op_apply (op);
	  gegl_object_destroy (GEGL_OBJECT(op)); 

	}

	gegl_object_destroy (GEGL_OBJECT(d)); 
	gegl_object_destroy (GEGL_OBJECT(cm)); 

  }
#endif

#if 1   /* This one uses the GeglImageBuffer and GeglOp directly */ 
  {

	GeglColorModel *cm = GEGL_COLOR_MODEL(
				gegl_color_model_rgb_float_new(FALSE,FALSE));
	GeglImageBuffer *image_buffer = gegl_image_buffer_new(cm,5,5);

	/* 
	   Create a graphics state we'll pass to the drawable once
	   we make our image buffer belong to one of those. 
	*/
	GeglGraphicsState *graphics_state = gegl_graphics_state_new (cm);


        /* Fill the whole 5 x 5 float image with RED */
        {
	  GeglOp *op;
	  GeglRect r;
	  GeglColor *c = gegl_color_new (cm);
          gegl_color_set_constant (c, COLOR_RED);

	  gegl_rect_set (&r, 0,0,5,5);
	  op = GEGL_OP (gegl_fill_op_new (image_buffer, &r, c));

	  gegl_op_apply (op);

	  gegl_object_destroy (GEGL_OBJECT(op)); 
	  gegl_object_destroy (GEGL_OBJECT(c)); 
        }

        /* Fill the subrect at (1,2) with size 2 x 2  with GREEN */
        {
	  GeglOp *op;
	  GeglRect r;

	  GeglColor *c = gegl_color_new (cm);
          gegl_color_set_constant (c, COLOR_BLUE);

	  gegl_rect_set (&r, 2,2,2,2);
	  op = GEGL_OP (gegl_fill_op_new (image_buffer, &r, c));

	  gegl_op_apply (op);

	  gegl_object_destroy (GEGL_OBJECT(op)); 
	  gegl_object_destroy (GEGL_OBJECT(c)); 

        }

        /* Fill the subrect at (0,0) with size 2 x 2  custom color */
        {
	  GeglOp *op;
	  GeglRect r;

	  GeglColor *c = gegl_color_new (cm);
	  GeglChannelValue * chans = gegl_color_get_channel_values(c);

	  /* The channels are actually funny unions */	
	  chans[0].f = .1;
	  chans[1].f = .2;
	  chans[2].f = .3;
       
	  gegl_rect_set (&r, 0,0,2,2);
	  op = GEGL_OP (gegl_fill_op_new (image_buffer, &r, c));

	  gegl_op_apply (op);

	  gegl_object_destroy (GEGL_OBJECT(op)); 
	  gegl_object_destroy (GEGL_OBJECT(c)); 

        }


        /* Print out the image values using the 
	   print operator 
        */
        {
          GeglOp *op;
	  GeglRect r;
	  gegl_rect_set (&r, 0,0,5,5);
	  op =  GEGL_OP (gegl_print_op_new (image_buffer, &r));
          gegl_op_apply (op);
	  gegl_object_destroy (GEGL_OBJECT(op)); 

	}

        gegl_object_destroy (GEGL_OBJECT(image_buffer)); 
        gegl_object_destroy (GEGL_OBJECT(cm)); 
  }
#endif

  return 0;
}
