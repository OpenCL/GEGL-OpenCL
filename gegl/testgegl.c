#include <gtk/gtk.h>
#include <gdk/gdkprivate.h>
#include <stdio.h>

#include "gegl-color.h"
#include "gegl-color-model-rgb-float.h"
#include "gegl-image-buffer.h"
#include "gegl-fill-op.h"
#include "gegl-print-op.h"
#include "gegl-utils.h"



int
main (int argc, char *argv[])
{  
  gtk_init (&argc, &argv);

#if 1 
  {

	GeglColorModel *cm = GEGL_COLOR_MODEL(
				gegl_color_model_rgb_float_new(FALSE));
	GeglImageBuffer *image_buffer = gegl_image_buffer_new(cm,5,5);


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
