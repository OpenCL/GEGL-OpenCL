#ifdef CHANT_SELF
 
#else

#define CHANT_FILTER
#define CHANT_NAME            stretch_contrast
#define CHANT_DESCRIPTION     "Scales the components of the buffer to be in the 0.0-1.0 range"

#define CHANT_SELF            "stretch-contrast.c"
#define CHANT_CLASS_CONSTRUCT /*< we need to modify the standard class init
                                  of the super class */
#include "gegl-chant.h"

static gboolean
inner_evaluate (gdouble        min,
                gdouble        max,
                guchar        *buf,
                gint           n_pixels)
{
  gint o;
  float *p = (float*) (buf);

  for (o=0; o<n_pixels; o++)
    {
      gint i;
      for (i=0;i<3;i++)
        p[i] = (p[i] - min) / (max-min);
      p+=4;
    }
  return TRUE;
}

static void
buffer_get_min_max (GeglBuffer *buffer,
                    gdouble    *min,
                    gdouble    *max)
{
  gdouble tmin = 9000000.0;
  gdouble tmax =-9000000.0;

  gfloat *buf = g_malloc0 (sizeof (gfloat) * 4 * buffer->width * buffer->height);
  gint i;
  gegl_buffer_get_fmt (buffer, buf, babl_format ("RGBA float"));
  for (i=0;i<gegl_buffer_pixels (buffer);i++)
    {
      gint c;
      for (c=0;c<3;c++)
        {
          gdouble val = buf[i*4+c];
          if (val<tmin)
            tmin=val;
          if (val>tmax)
            tmax=val;
        }
    }
  g_free (buf);
  if (min)
    *min = tmin;
  if (max)
    *max = tmax;
}

static gboolean
evaluate (GeglOperation *operation,
          const gchar   *output_prop)
{
  GeglOperationFilter *filter = GEGL_OPERATION_FILTER (operation);
  GeglRect            *result;
  GeglBuffer          *input,
                      *output;
  gdouble              min, max;

  input = filter->input;
  result = gegl_operation_need_rect (operation);

  if (result->w==0 ||
      result->h==0)
    {
      output = g_object_ref (input);
      return TRUE;
    }

  buffer_get_min_max (input, &min, &max);

  output = g_object_new (GEGL_TYPE_BUFFER,
                         "format", babl_format ("RGBA float"),
                         "x",      result->x,
                         "y",      result->y,
                         "width",  result->w,
                         "height", result->h,
                         NULL);

  {
    gint row;
    guchar *buf;
    gint chunk_size=128;
    gint consumed=0;

    buf = g_malloc0 (sizeof (gfloat) * 4 * result->w * chunk_size);

    for (row=0;row<result->h;row=consumed)
      {
        gint chunk = consumed+chunk_size<result->h?chunk_size:result->h-consumed;
        GeglBuffer *in_line = g_object_new (GEGL_TYPE_BUFFER,
                                            "source", input,
                                            "x",      result->x,
                                            "y",      result->y+row,
                                            "width",  result->w,
                                            "height", chunk,
                                            NULL);
        GeglBuffer *out_line = g_object_new (GEGL_TYPE_BUFFER,
                         "source", output,
                         "x",      result->x,
                         "y",      result->y+row,
                         "width",  result->w,
                         "height", chunk,
                         NULL);
        gegl_buffer_get_fmt (in_line, buf, babl_format ("RGBA float"));
        inner_evaluate (min, max, buf, result->w * chunk);
        gegl_buffer_set_fmt (out_line, buf, babl_format ("RGBA float"));
        g_object_unref (in_line);
        g_object_unref (out_line);
        consumed+=chunk;
      }
    g_free (buf);
  }

  if (filter->output)
    g_object_unref (filter->output);
  filter->output = output;

  return TRUE;
}

/* computes the bounding rectangle of the raster data needed to
 * compute the scale op.
 */
static gboolean
calc_need_rect (GeglOperation *self)
{
  GeglRect *requested;
  requested = gegl_operation_need_rect (self);

  gegl_operation_set_need_rect (self, "input",
      gegl_operation_have_rect (self)->x,
      gegl_operation_have_rect (self)->y,
      gegl_operation_have_rect (self)->w,
      gegl_operation_have_rect (self)->h);
  return TRUE;
}

/* This is called at the end of the gobject class_init function, the
 * CHANT_CLASS_CONSTRUCT was needed to make this happen
 *
 * Here we override the standard passthrough options for the rect
 * computations.
 */
static void class_init (GeglOperationClass *operation_class)
{
  operation_class->calc_need_rect = calc_need_rect;
}

#endif
