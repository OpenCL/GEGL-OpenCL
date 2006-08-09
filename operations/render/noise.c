#ifdef CHANT_SELF
 
chant_float (alpha, -100.0, 100.0, 12.3, "")
chant_float (beta,  -100.0, 100.0, 0.1, "")
chant_float (zoff, -100.0, 100.0,  -1, "")
chant_float (seed,  -100.0, 100.0, 20.0, "")
chant_float (n,     -100.0, 100.0, 6.0, "")

#else

#define CHANT_SOURCE
#define CHANT_NAME           perlin_noise
#define CHANT_DESCRIPTION    "Perlin noise generator"

#define CHANT_SELF           "noise.c"
#define CHANT_CLASS_CONSTRUCT
#include "gegl-chant.h"
#include "perlin/perlin.c"
#include "perlin/perlin.h"

static gboolean
evaluate (GeglOperation *operation,
          const gchar   *output_prop)
{
  GeglRect  *need;
  GeglOperationSource     *op_source = GEGL_OPERATION_SOURCE(operation);
  ChantInstance *self = CHANT_INSTANCE (operation);

  if(strcmp("output", output_prop))
    return FALSE;

  if (op_source->output)
    g_object_unref (op_source->output);
  op_source->output=NULL;

  need = gegl_operation_need_rect (operation);

  {
    GeglRect *result = gegl_operation_result_rect (operation);
    gfloat *buf;

    op_source->output = g_object_new (GEGL_TYPE_BUFFER,
                        "format", 
                        babl_format ("Y float"),
                        "x",      result->x,
                        "y",      result->y,
                        "width",  result->w,
                        "height", result->h,
                        NULL);
    buf = g_malloc (gegl_buffer_pixels (op_source->output) * gegl_buffer_px_size (op_source->output));
      {
        gfloat *dst=buf;
        gint y;
        for (y=0; y < result->h; y++)
          {
            gint x;
            for (x=0; x < result->w; x++)
              {
                gfloat val;

                val = PerlinNoise3D ((double) (x + result->x)/50.0,
                                     (double) (y + result->y)/50.0,
                                     (double) self->zoff, self->alpha, self->beta,
                                     self->n);
                *dst = val * 0.5 + 0.5;
                dst ++;
              }
          }
      }
    gegl_buffer_set (op_source->output, buf);
    g_free (buf);
  }
  return  TRUE;
}


static gboolean
calc_have_rect (GeglOperation *operation)
{
  /*OpNoise    *self      = (OpNoise*)(operation);*/

  gegl_operation_set_have_rect (operation, -1000000, -1000000, 2000000, 2000000);

  return TRUE;
}

static void dispose (GObject *gobject)
{
  /*OpNoise *self = (OpNoise*)(gobject);*/
  G_OBJECT_CLASS (g_type_class_peek_parent (G_OBJECT_GET_CLASS (gobject)))->dispose (gobject);
}

static void class_init (GeglOperationClass *klass)
{
  G_OBJECT_CLASS (klass)->dispose = dispose;
}

#endif
