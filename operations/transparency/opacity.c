#ifdef CHANT_SELF
chant_float (value, -100.0, 100.0, 0.5, "global opacity value, used if no aux input is provided")
#else

#define CHANT_POINT_COMPOSER
#define CHANT_NAME            opacity
#define CHANT_DESCRIPTION     "weights the opacity of the input with either the value of the aux input or the global value property"
#define CHANT_SELF            "opacity.c"
#define CHANT_CONSTRUCT
#include "gegl-chant.h"

static void init (ChantInstance *self)
{
  GEGL_OPERATION_POINT_COMPOSER (self)->format = babl_format ("RaGaBaA float");
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
          gint j;
          for (j=0; j<4; j++)
            out[j] = in[j] * value;
          in  += 4;
          out += 4;
        }
    }
  else
    {
      for (i=0; i<n_pixels; i++)
        {
          gint j;
          for (j=0; j<4; j++)
            out[j] = in[j] * (*aux);
          in  += 4;
          out += 4;
          aux += 1;
        }
    }
  return TRUE;
}
#endif
