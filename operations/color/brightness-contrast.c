#ifdef CHANT_SELF
 
chant_float (contrast,   -100.0, 100.0, 1.0, "Range scale factor")
chant_float (brightness,  -10.0,  10.0, 0.0, "Amount to increase brightness")

#else

#define CHANT_POINT_FILTER
#define CHANT_NAME         brightness_contrast
#define CHANT_DESCRIPTION  "Changes the light level and contrast."
#define CHANT_SELF         "brightness-contrast.c"
#include "gegl-chant.h"

static gboolean
evaluate (GeglOperation *op,
          void          *in_buf,
          void          *out_buf,
          glong          n_pixels)
{
  ChantInstance *self = CHANT_INSTANCE (op);
  gint o;
  gfloat *p = in_buf;  /* it is inplace anyways, and out_but==in_buf) */

  g_assert (in_buf == out_buf);

  for (o=0; o<n_pixels; o++)
    {
      gint i;
      for (i=0;i<3;i++)
        p[i] = (p[i] - 0.5) * self->contrast + self->brightness + 0.5;
      p+=4;
    }
  return TRUE;
}
#endif
