#ifdef CHANT_SELF

chant_float (value, -100.0, 100.0, 0.5, "global threshold level (used when there is no aux input")

#else

#define CHANT_POINT_COMPOSER
#define CHANT_NAME            threshold
#define CHANT_DESCRIPTION     "thresholds the image to white/black based on either the global value set in the value property, or per pixel from the aux input"
#define CHANT_SELF            "threshold.c"
#define CHANT_CONSTRUCT
#include "gegl-chant.h"

static void init (ChantInstance *self)
{
  GEGL_OPERATION_POINT_COMPOSER (self)->format = babl_format ("YA float");
  GEGL_OPERATION_POINT_COMPOSER (self)->aux_format = babl_format ("Y float");
}

static gboolean
evaluate (GeglOperation *op,
          void          *in_buf,
          void          *aux_buf,
          void          *out_buf,
          glong          n_pixels)
{
  gint i;

  gfloat *in = in_buf;
  gfloat *out = out_buf;
  gfloat *aux = aux_buf;

  if (aux == NULL)
    {
      gfloat value = CHANT_INSTANCE (op)->value;
      for (i=0; i<n_pixels; i++)
        {
          out[0] = in[0]>=value?1.0:0.0;
          out[1] = in[1];
          in  += 2;
          out += 2;
        }
    }
  else
    {
      for (i=0; i<n_pixels; i++)
        {
          out[0] = in[0]>=(*aux)?1.0:0.0;
          out[1] = in[1];
          in  += 2;
          out += 2;
          aux += 1;
        }
    }
  return TRUE;
}
#endif
