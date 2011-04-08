#include "config.h"
#include <glib/gi18n-lib.h>

#ifdef GEGL_CHANT_PROPERTIES

gegl_chant_int (amplitude, _("Amplitude"), 0, 200, 50,
   _("Amplitude of ripple."))

gegl_chant_int (period, _("Period"), 0, 200, 50,
   _("Period of ripple."))

gegl_chant_double (phi, _("Phase Shift"), -1.0, 1.0, 0.0,
   _("As proportion of pi."))

gegl_chant_boolean (antialias,   _("Antialiasing"),   FALSE,
                    _("Antialiasing"))

#else

#define GEGL_CHANT_TYPE_AREA_FILTER
#define GEGL_CHANT_C_FILE       "ripple-test.c"

#include "gegl-chant.h"
#include <stdio.h>
#include <math.h>


static gdouble
displace_amount (gint location, gint amplitude, gint period, gdouble phi)
{
  return ((gdouble)amplitude *
	  sin (2 * G_PI * (((gdouble)location / (gdouble)period) + G_PI*phi)));
}

static void
horizontal_shift (GeglBuffer          *src,
      const GeglRectangle *src_rect,
      GeglBuffer          *dst,
      const GeglRectangle *dst_rect,
      gint amplitude,
      gint period,
      gdouble phi,
      gboolean antialias)
{
  gint u,v;
  gint src_offset, dst_offset;
  gfloat *src_buf;
  gfloat *dst_buf;
  src_buf = g_new0 (gfloat, src_rect->width * src_rect->height * 4);
  dst_buf = g_new0 (gfloat, dst_rect->width * dst_rect->height * 4);
  gegl_buffer_get (src, 1.0, src_rect, babl_format ("RaGaBaA float"), src_buf, GEGL_AUTO_ROWSTRIDE);
  src_offset = 0;
  dst_offset = 0;
  for (v=0; v<dst_rect->height; v++)
    {
      gdouble shift = displace_amount(v+dst_rect->y, amplitude, period, phi);
      gdouble floor_shift = floor(shift);
      gdouble remainder = shift-floor_shift;
      gdouble temp_value;
      
      for (u=0; u<(dst_rect->width); u++)
        {
          gint i;
      
          for (i=0; i<4; i++)
            {
              gint new_pixel = src_offset + (4*amplitude + 4*amplitude*2*v + 4*(gint)floor_shift);
              //antialias

      
              temp_value = src_buf[new_pixel];

              if(antialias && u>3 && i < 3)
                {
                  gint upper_pixel = new_pixel - 4;
                  temp_value = (remainder)*src_buf[new_pixel] + (1.0-remainder)*src_buf[upper_pixel];
      
                }
      
              dst_buf[dst_offset] = temp_value;
              src_offset++;
              dst_offset++;
            }
        }
    }

  gegl_buffer_set (dst, dst_rect, babl_format ("RaGaBaA float"), dst_buf, GEGL_AUTO_ROWSTRIDE);
  g_free (src_buf);
  g_free (dst_buf);
}

static void prepare (GeglOperation *operation)
{
  GeglChantO              *o;
  GeglOperationAreaFilter *op_area;

  op_area = GEGL_OPERATION_AREA_FILTER (operation);
  o       = GEGL_CHANT_PROPERTIES (operation);

    op_area->left   =
    op_area->right  = o->amplitude;
    //op_area->top    =
    //op_area->bottom = 

  gegl_operation_set_format (operation, "output",
                             babl_format ("RaGaBaA float"));

}

static gboolean
process (GeglOperation       *operation,
         GeglBuffer          *input,
         GeglBuffer          *output,
         const GeglRectangle *result)
{

  GeglRectangle rect;
  GeglChantO *o = GEGL_CHANT_PROPERTIES (operation);
  GeglOperationAreaFilter *op_area;
  op_area = GEGL_OPERATION_AREA_FILTER (operation);

  rect = *result;

  rect.x-=op_area->left;
  rect.width+=op_area->left + op_area->right;
  horizontal_shift(input, &rect, output, result, o->amplitude, o->period, o->phi, o->antialias);
  return  TRUE;
}


static void
gegl_chant_class_init (GeglChantClass *klass)
{
  GeglOperationClass       *operation_class;
  GeglOperationFilterClass *filter_class;

  operation_class = GEGL_OPERATION_CLASS (klass);
  filter_class    = GEGL_OPERATION_FILTER_CLASS (klass);

  filter_class->process    = process;
  operation_class->prepare = prepare;

  operation_class->categories  = "blur";
  operation_class->name        = "gegl:ripple-test";
  operation_class->description =
       _("Trying out GEGL.");
}

#endif
