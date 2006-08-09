#ifdef CHANT_SELF
   /* no properties */
#else

#define CHANT_POINT_FILTER
#define CHANT_NAME          invert
#define CHANT_DESCRIPTION   "inverts the components (except alpha, one by one)"
#define CHANT_SELF          "invert.c"
#define CHANT_CONSTRUCT
#include "gegl-chant.h"

static void init (ChantInstance *self)
{
  /* set the babl format this operation prefers to work on */
  GEGL_OPERATION_POINT_FILTER (self)->format = babl_format ("RGBA float");
}

static gboolean
evaluate (GeglOperation *op,
          void          *in_buf,
          void          *out_buf,
          glong          samples) 
{
  gint i;
  gfloat *in  = in_buf;
  gfloat *out = out_buf;

  for (i=0; i<samples; i++)
    {
      int  j;
      for (j=0; j<3; j++)
        {
          out[j] = 1.0 - in[j];
        }
      out[3]=in[3];
      in += 4;
      out+= 4;
    }
  return TRUE;
}

#endif
