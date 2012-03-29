/* This file is an image processing operation for GEGL
 *
 * GEGL is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * GEGL is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with GEGL; if not, see <http://www.gnu.org/licenses/>.
 *
 * Copyright        2010 Danny Robson      <danny@blubinc.net>
 * (pfscalibration) 2006 Grzegorz Krawczyk <gkrawczyk@users.sourceforge.net>
 */

#include "config.h"
#include <glib/gi18n-lib.h>

#ifdef GEGL_CHANT_PROPERTIES

gegl_chant_string(exposures, _("Exposure Values"), "",
                  _("Relative brightness of each exposure in EV"))
gegl_chant_int   (steps, _("Discretization Bits"),
                  8, 32, 12,
                  _("Log2 of source's discretization steps"))
gegl_chant_double (sigma, _("Weight Sigma"),
                  0.0f, 32.0f, 8.0f,
                  _("Weight distrubtion sigma controlling response contributions"))

#else

/*#define DEBUG_LINEARIZE*/
/*#define DEBUG_SAVE_CURVES*/

#include <gegl-plugin.h>
struct _GeglChant
{
  GeglOperationFilter parent_instance;
  gpointer            properties;
};

typedef struct
{
  GeglOperationFilterClass parent_class;
} GeglChantClass;


#define GEGL_CHANT_C_FILE "exp-combine.c"
#include "gegl-chant.h"
GEGL_DEFINE_DYNAMIC_OPERATION(GEGL_TYPE_OPERATION_FILTER)

#include <errno.h>
#include <math.h>
#include <stdio.h>

#include "gegl-debug.h"
#include "graph/gegl-node.h"
#include "graph/gegl-pad.h"


static const gchar *PAD_FORMAT = "R'G'B' float";
static const gchar *EXP_PREFIX = "exposure-";

enum
{
  PROP_OUTPUT = 1,
  PROP_INPUT,
  PROP_AUX
};


/* maximum iterations after algorithm accepts local minima */
static const guint MAXIT = 500;

/* maximum accepted error */
static const gfloat MAX_DELTA  = 1e-5f;
static const gfloat MIN_WEIGHT = 1e-3f;

/* number of pixels used if downscaling for curve estimation */
static const guint  MAX_SCALED_PIXELS = 1000 * 1000;

typedef enum
{
  PIXELS_ACTIVE,    /* Must be lowest valid ID, zero */
  PIXELS_FULL,
  PIXELS_SCALED,

  NUM_PIXEL_BUCKETS
} pixel_bucket;


typedef struct _exposure
{
  /* Immediately higher and lower exposures */
  struct _exposure *hi;
  struct _exposure *lo;

  gfloat *pixels[NUM_PIXEL_BUCKETS];

  /* 'Duration' of exposure. May only be relatively correct against the
   * other exposures in sequence.
   */
  gfloat  ti;
} exposure;


static void
gegl_expcombine_exposures_set_active (GSList       *exposures,
                                      pixel_bucket  bucket)
{
  g_return_if_fail (bucket > PIXELS_ACTIVE && bucket < NUM_PIXEL_BUCKETS);

  for (; exposures; exposures = exposures->next)
    {
      exposure *e              = exposures->data;
      e->pixels[PIXELS_ACTIVE] = e->pixels[bucket];
    }
}


/* Comparator for struct exposure: order by ti */
static int
gegl_expcombine_exposure_cmp (gconstpointer _a,
                              gconstpointer _b)
{
  const exposure *a = _a,
                 *b = _b;

  if (a->ti > b->ti)
    return  1;
  else if (a->ti < b->ti)
    return -1;
  else
    return  0;
}


static exposure*
gegl_expcombine_new_exposure (void)
{
  exposure *e = g_new (exposure, 1);
  e->hi = e->lo = e;

  memset (e->pixels, 0, sizeof (e->pixels));
  e->ti = NAN;

  return e;
}


static void
gegl_expcombine_destroy_exposure (exposure *e)
{
  guint i, j;

  /* Unlink ourselves from the next hi and lo exposures */
  g_return_if_fail (e->lo);
  g_return_if_fail (e->hi);

  e->lo->hi = (e->hi == e) ? e->lo : e->hi;
  e->hi->lo = (e->lo == e) ? e->hi : e->lo;

  /* Free each pixel bucket. Each time a non-null is found, free it then null
   * any references to the same pixels in later buckets.
   */
  for (i = PIXELS_ACTIVE + 1; i < NUM_PIXEL_BUCKETS; ++i)
    {
      if (e->pixels[i])
        {
          g_free (e->pixels[i]);

          for (j = i + 1; j < NUM_PIXEL_BUCKETS; ++j)
              if (e->pixels[j] == e->pixels[i])
                  e->pixels[j] = NULL;
        }
    }

  g_free (e);
}


/* Initialise a response curve to linear. The absolute values are probably
 * irrelevant as the curve should be normalised later.
 */
static void
gegl_expcombine_response_linear (gfloat *response,
                                 guint   steps)
{
  guint i;
  for (i = 0; i < steps; ++i)
    response[i] = i / 255.0f;
}


/* Generate a weighting curve for the range of pixel values. Weights pixels in
 * the centre of the range more highly than the extremes.
 *
 * -sigma              weights the exponential arguments
 * -step_min,step_max  describes the region we provide weighting for
 */
static void
gegl_expcombine_weights_gauss (gfloat *weights,
                               guint   steps,
                               guint   step_min,
                               guint   step_max,
                               gfloat  sigma)
{
  gfloat step_mid1, step_mid2;
  gfloat weight;
  guint i;

  g_return_if_fail (weights);
  g_return_if_fail (step_min <= step_max);
  g_return_if_fail (step_min <  steps);
  g_return_if_fail (step_max <  steps);

  step_mid1 = step_min + (step_max - step_min) / 2.0f - 0.5f;
  step_mid2 = (step_mid1 - step_min) * (step_mid1 - step_min);

  for (i = 0; i < steps; ++i)
    {
      if (i < step_min || i > step_max)
        {
          weights[i] = 0.0f;
          continue;
        }

      /* gkrawczyk: that's not really a gaussian, but equation is
       * taken from Robertson02 paper.
       */
      weight = exp ( -sigma                 *
                    ((gfloat)i - step_mid1) *
                    ((gfloat)i - step_mid1) /
                     step_mid2);

      /* ignore very low weights. we rely on these weights being set to zero
       * in the response estimation code, thus this should not be removed.
       */
      if (weight < MIN_WEIGHT)
        weights[i] = 0.0f;
      else
        weights[i] = weight;
    }
}


/* Normalise the response curve, in place, using the midpoint of the non-zero
 * region of the response curve. Returns the mid-point value of the curve.
 */
static gfloat
gegl_expcombine_normalize (gfloat *response,
                           guint   steps)
{
  guint  step_min, step_max, step_mid;
  guint  i;
  gfloat val_mid;

  g_return_val_if_fail (response, NAN);
  g_return_val_if_fail (steps > 0, NAN);

  /* Find the first and last non-zero values in response curve */
  for (step_min = 0;
       step_min < steps && response[step_min] == 0;
       ++step_min)
    ;
  for (step_max = steps - 1;
       step_max > 0 && response[step_max] == 0;
       --step_max)
    ;

  g_return_val_if_fail (step_max >= step_min, NAN);
  step_mid = step_min + (step_max - step_min) / 2;

  /* Find the non-zero mid-value of the response curve */
  val_mid = response[step_mid];
  if (val_mid == 0.0f)
  {
    /* find first non-zero middle response */
    while (step_mid < step_max && response[step_mid] == 0.0f)
        ++step_mid;
    val_mid = response[step_mid];
  }

  /* Normalize response curve values via the mid-value */
  g_return_val_if_fail (val_mid != 0.0f, 0.0f);

  for (i = 0; i < steps; ++i)
      response[i] /= val_mid;

  return val_mid;
}


/* Apply debevec model for response curve application. Does not suffer from
 * apparent discolouration in some areas of image reconstruction when
 * compared with robertson.
 */
static int
gegl_expcombine_apply_debevec  (gfloat              *hdr,
                                const guint          components,
                                GSList              *imgs,
                                gfloat             **response,
                                const gfloat        *weighting,
                                const guint          steps,
                                const GeglRectangle *extent)
{
  guint  step_min, step_max;
  guint  num_imgs    = g_slist_length (imgs);
  guint  saturated   = 0;
  guint  pixel_count = (guint)(extent->width * extent->height);
  guint  i, j;

  g_return_val_if_fail (hdr,                        G_MAXINT);
  g_return_val_if_fail (g_slist_length (imgs) > 0,  G_MAXINT);
  g_return_val_if_fail (response,                   G_MAXINT);
  g_return_val_if_fail (weighting,                  G_MAXINT);
  g_return_val_if_fail (steps > 0,                  G_MAXINT);
  g_return_val_if_fail (extent,                     G_MAXINT);
  g_return_val_if_fail (extent->width  > 0,         G_MAXINT);
  g_return_val_if_fail (extent->height > 0,         G_MAXINT);

  /* anti saturation: calculate trusted camera output range */
  for (step_min = 0, i = step_min; i <    steps; ++i)
    if (weighting[i] > 0)
      {
        step_min = i;
        break;
      }
  for (step_max = steps - 1, i = step_max; i > step_min; --i)
    if (weighting[i] > 0)
      {
        step_max = i;
        break;
      }
  g_return_val_if_fail (step_max >= step_min, G_MAXINT);

  for (j = 0; j < pixel_count; ++j)
    {
      gfloat  sum[3]  = { 0.0f, 0.0f, 0.0f },
              div     = 0.0f;
      gfloat  ti_max  = G_MINFLOAT,
              ti_min  = G_MAXFLOAT;
      gfloat  average;
      guint   white_step[3], black_step[3];

      /* all exposures for each pixel */
      for (i = 0; i < num_imgs; ++i)
        {
          exposure *exp_i;
          guint     step[3], step_hi[3], step_lo[3];

          exp_i    = g_slist_nth_data (imgs, i);
          step[0]  = exp_i->pixels[PIXELS_ACTIVE][0 + j * components];
          step[1]  = exp_i->pixels[PIXELS_ACTIVE][1 + j * components];
          step[2]  = exp_i->pixels[PIXELS_ACTIVE][2 + j * components];
          g_return_val_if_fail (step[0] < steps && step[1] < steps && step[2] < steps, G_MAXINT);

          /* anti saturation: observe minimum exposure time at which saturated
           * value is present, and maximum exp time at which black value is
           * present
           */
          if ((step[0] > step_max ||
               step[1] > step_max ||
               step[2] > step_max) && exp_i->ti < ti_min)
            {
              white_step[0] = MIN (step[0], steps);
              white_step[1] = MIN (step[1], steps);
              white_step[2] = MIN (step[2], steps);
              ti_min        = exp_i->ti;
            }

          if ((step[0] < step_min ||
               step[1] < step_min ||
               step[2] < step_min) && exp_i->ti > ti_max)
            {
              /* This could be protected by a max (0, step), but we're using
               * unsigned ints so it would be worthless currently.
               */
              black_step[0] = step[0];
              black_step[1] = step[1];
              black_step[2] = step[2];
              ti_max        = exp_i->ti;
            }

          /* anti ghosting: monotonous increase in time should result in
           * monotonous increase in intensity; make forward and backward check,
           * ignore value if condition not satisfied
           */
          step_lo[0] = exp_i->lo->pixels[PIXELS_ACTIVE][0 + j * components];
          step_lo[1] = exp_i->lo->pixels[PIXELS_ACTIVE][1 + j * components];
          step_lo[2] = exp_i->lo->pixels[PIXELS_ACTIVE][2 + j * components];
          g_return_val_if_fail (step_lo[0] < steps &&
                                step_lo[1] < steps &&
                                step_lo[2] < steps, G_MAXINT);

          step_hi[0] = exp_i->hi->pixels[PIXELS_ACTIVE][0 + j * components];
          step_hi[1] = exp_i->hi->pixels[PIXELS_ACTIVE][1 + j * components];
          step_hi[2] = exp_i->hi->pixels[PIXELS_ACTIVE][2 + j * components];
          g_return_val_if_fail (step_hi[0] < steps &&
                                step_hi[1] < steps &&
                                step_hi[2] < steps, G_MAXINT);

          if (step_lo[0] > step[0] ||
              step_lo[1] > step[1] ||
              step_lo[2] > step[2])
            {
              white_step[0] = step[0];
              white_step[1] = step[1];
              white_step[2] = step[2];
              /* TODO: This is not an fminf in luminance. Should it be? */
              /* ti_min        = fminf (exp_i->ti, ti_min); */
              continue;
            }

          if (step_hi[0] < step[0] ||
              step_hi[1] < step[1] ||
              step_hi[2] < step[2])
            {
              black_step[0] = step[0];
              black_step[1] = step[1];
              black_step[2] = step[2];
              /* TODO: This is not an fminf in luminance. Should it be? */
              /* ti_max        = fmaxf (exp_i->ti, ti_max); */
              continue;
            }

          /* Weight the addition into each channel by the average weight of
           * the channels and the exposure duration. This is the key
           * difference to robertson.
           */
          average  = weighting [step[0]] +
                     weighting [step[1]] +
                     weighting [step[2]];
          average /= 3.0f;

          sum[0] += response[0][step[0]] * average / exp_i->ti;
          sum[1] += response[1][step[1]] * average / exp_i->ti;
          sum[2] += response[2][step[2]] * average / exp_i->ti;

          div += average;
        }

      g_return_val_if_fail (sum[0] >= 0.0f, G_MAXINT);
      g_return_val_if_fail (sum[1] >= 0.0f, G_MAXINT);
      g_return_val_if_fail (sum[2] >= 0.0f, G_MAXINT);
      g_return_val_if_fail (   div >= 0.0f, G_MAXINT);
      g_return_val_if_fail (ti_max <= ti_min, G_MAXINT);

      /* anti saturation: if a meaningful representation of pixel was not
       * found, replace it with information from observed data
       */
      if (div == 0.0f)
          ++saturated;

      if (div == 0.0f && ti_max != G_MINFLOAT)
        {
          sum[0] = response[0][black_step[0]] / ti_max;
          sum[1] = response[1][black_step[1]] / ti_max;
          sum[2] = response[2][black_step[2]] / ti_max;
          div = 1.0f;
        }
      else if (div == 0.0f && ti_min != G_MAXFLOAT)
        {
          sum[0] = response[0][white_step[0]] / ti_min;
          sum[1] = response[1][white_step[1]] / ti_min;
          sum[2] = response[2][white_step[2]] / ti_min;
          div = 1.0f;
        }

      if (div == 0.0f)
        {
          hdr[0 + j * components] = 0.0f;
          hdr[1 + j * components] = 0.0f;
          hdr[2 + j * components] = 0.0f;
        }
      else
        {
          hdr[0 + j * components] = sum[0] / div;
          hdr[1 + j * components] = sum[1] / div;
          hdr[2 + j * components] = sum[2] / div;
        }
    }

  return saturated;
}


/**
 * @brief Create HDR image by applying response curve to given images
 * taken with different exposures
 *
 * @param hdr [out] HDR image
 * @param imgs      list of scene exposures
 * @param response  camera response function (array size of `steps')
 * @param weighting weighting function for camera output values (array size of `steps')
 * @param steps     number of camera output levels
 * @return          number of saturated pixels in the HDR image (0: all OK)
 */
static int
gegl_expcombine_apply_response (gfloat              *hdr,
                                const guint          offset,
                                const guint          components,
                                GSList              *imgs,
                                const gfloat        *response,
                                const gfloat        *weighting,
                                const guint          steps,
                                const GeglRectangle *extent)
{
  guint  step_min, step_max;
  guint  num_imgs  = g_slist_length (imgs);
  guint  saturated = 0;
  guint  pixel_count = (guint)(extent->width * extent->height);
  guint  i, j;

  g_return_val_if_fail (hdr,                        G_MAXINT);
  g_return_val_if_fail (g_slist_length (imgs) > 0,  G_MAXINT);
  g_return_val_if_fail (response,                   G_MAXINT);
  g_return_val_if_fail (weighting,                  G_MAXINT);
  g_return_val_if_fail (steps > 0,                  G_MAXINT);
  g_return_val_if_fail (extent,                     G_MAXINT);
  g_return_val_if_fail (extent->width  > 0,         G_MAXINT);
  g_return_val_if_fail (extent->height > 0,         G_MAXINT);

  /* anti saturation: calculate trusted camera output range */
  for (step_min = 0, i = step_min; i < steps; ++i)
    {
      if (weighting[i] > 0)
        {
          step_min = i;
          break;
        }
    }
  for (step_max = steps - 1, i = step_max; i > step_min; --i)
    {
      if (weighting[i] > 0)
        {
          step_max = i;
          break;
        }
    }

  g_return_val_if_fail (step_max >= step_min, G_MAXINT);

  for (j = 0; j < pixel_count; ++j)
    {
      gfloat  sum    = 0.0f,
              div    = 0.0f;
      gfloat  ti_max = G_MINFLOAT,
              ti_min = G_MAXFLOAT;

      /* all exposures for each pixel */
      for (i = 0; i < num_imgs; ++i)
        {
          exposure *exp_i;
          guint     step, step_hi, step_lo;

          exp_i = g_slist_nth_data (imgs, i);
          step  = exp_i->pixels[PIXELS_ACTIVE][offset + j * components];
          g_return_val_if_fail (step < steps, G_MAXINT);

          /* anti saturation: observe minimum exposure time at which saturated
           * value is present, and maximum exp time at which black value is
           * present
           */
          if (step > step_max)
              ti_min = fminf (ti_min, exp_i->ti);
          if (step < step_min)
              ti_max = fmaxf (ti_max, exp_i->ti);

          /* anti ghosting: monotonous increase in time should result in
           * monotonous increase in intensity; make forward and backward check,
           * ignore value if condition not satisfied
           */
          step_lo = exp_i->lo->pixels[PIXELS_ACTIVE][offset + j * components];
          step_hi = exp_i->hi->pixels[PIXELS_ACTIVE][offset + j * components];

          if (step_lo > step || step_hi < step)
               continue;

          sum += weighting[step] * exp_i->ti * response[step];
          div += weighting[step] * exp_i->ti * exp_i->ti;
        }

      g_return_val_if_fail (sum >= 0.0f, G_MAXINT);
      g_return_val_if_fail (div >= 0.0f, G_MAXINT);
      g_return_val_if_fail (ti_max <= ti_min, G_MAXINT);

      /* anti saturation: if a meaningful representation of pixel was not
       * found, replace it with information from observed data
       */
      if (div == 0.0f)
          ++saturated;

      if (div == 0.0f && ti_max != G_MINFLOAT)
        {
          sum = response[step_min];
          div = ti_max;
        }

      if (div == 0.0f && ti_min != G_MAXFLOAT)
        {
          sum = response[step_max];
          div = ti_min;
        }

      if (div != 0.0f)
          hdr[offset + j * components] = sum / div;
      else
          hdr[offset + j * components] = 0.0f;
    }

  return saturated;
}


/**
 * @brief Calculate camera response using Robertson02 algorithm
 *
 * @param luminance [out] estimated luminance values
 * @param imgs            list of scene exposures
 * @param response  [out] array to put response function
 * @param weighting       weights
 * @param steps           max camera output (no of discrete steps)
 * @return                number of saturated pixels in the HDR image (0: all OK)
 */

static int
gegl_expcombine_get_response (gfloat              *hdr,
                              const guint          offset,
                              const guint          components,
                              GSList              *imgs,
                              gfloat              *response,
                              const gfloat        *weighting,
                              guint                steps,
                              const GeglRectangle *extent)
{
  gfloat  *response_old;
  gfloat   delta, delta_old;

  gulong  *card;
  gfloat  *sum;

  guint    i, j, hits;
  guint    iterations;
  guint    pixel_count = (guint)(extent->height * extent->width);
  gulong   saturated  = 0;
  gboolean converged;

  g_return_val_if_fail (hdr,                        G_MAXINT);
  g_return_val_if_fail (g_slist_length (imgs) > 1,  G_MAXINT);
  g_return_val_if_fail (response,                   G_MAXINT);
  g_return_val_if_fail (weighting,                  G_MAXINT);
  g_return_val_if_fail (steps > 0,                  G_MAXINT);
  g_return_val_if_fail (extent,                     G_MAXINT);
  g_return_val_if_fail (extent->width  > 0,         G_MAXINT);
  g_return_val_if_fail (extent->height > 0,         G_MAXINT);

  response_old = g_new (gfloat, steps);

  /* 0. Initialization */
  gegl_expcombine_normalize (response, steps);
  for (i = 0; i < steps; ++i)
      response_old[i] = response[i];

  saturated = gegl_expcombine_apply_response (hdr, offset, components, imgs,
                                              response, weighting, steps,
                                              extent);

  converged  = FALSE;
  card       = g_new (gulong, steps);
  sum        = g_new (gfloat, steps);
  iterations = 0;
  delta_old  = 0.0f;

  /* Optimization process */
  while (!converged)
  {
    GSList *cursor = imgs;
    /* 1. Minimize with respect to response */
    for (i = 0; i < steps; ++i)
      {
        card[i] = 0;
        sum [i] = 0.0f;
      }

    for (cursor = imgs; cursor; cursor = cursor->next)
      {
        exposure *e = cursor->data;

        for (j = 0; j < pixel_count; ++j)
          {
            guint step = e->pixels[PIXELS_ACTIVE][offset + j * components];
            if (step < steps)
              {
                sum[step] += e->ti * hdr[offset + j * components];
                ++card[step];
              }
            else
              g_warning ("robertson02: m out of range: %u", step);
          }
      }

    for (i = 0; i < steps; ++i)
      {
        if (card[i] != 0)
          response[i] = sum[i] / card[i];
        else
          response[i] = 0.0f;
      }

    /* 2. Apply new response */
    gegl_expcombine_normalize (response, steps);
    saturated = gegl_expcombine_apply_response (hdr, offset, components, imgs,
                                                response, weighting, steps,
                                                extent);

    /* 3. Check stopping condition */
    delta = 0.0f;
    hits  = 0;

    for (i = 0; i < steps; ++i)
      {
        g_return_val_if_fail (response[i] >= 0, G_MAXINT);

        if (response[i] != 0.0f)
          {
            gdouble diff     = response[i] - response_old[i];
            delta           += diff * diff;
            response_old[i]  = response[i];
            ++hits;
          }
      }
    delta /= hits;


    GEGL_NOTE (GEGL_DEBUG_PROCESS,
               "exp-combine convergence, #%u delta=%f (coverage: %u%%)",
               iterations,
               delta,
               (guint)(100.0f * (gfloat)hits / steps));
    if (delta < MAX_DELTA)
      converged = TRUE;
    else if (isnan (delta) || (iterations > MAXIT && delta_old < delta))
      {
        g_warning ("exp-combine failed to converge. too noisy data in range.");
        break;
      }

    delta_old = delta;
    ++iterations;
  }

  g_free (response_old);
  g_free (card);
  g_free (sum);

  return saturated;
}


/* All exposure pads have the common prefix, followed by a numeric
 * identifier.
 */
static inline gboolean
gegl_expcombine_is_exposure_padname (const gchar *p)
{
  return g_str_has_prefix (p, EXP_PREFIX);
}


static inline gboolean
gegl_expcombine_is_exposure_pad (GeglPad *p)
{
  return gegl_expcombine_is_exposure_padname (gegl_pad_get_name (p));
}


/* XXX: We create a large quantity of pads for all the exposures, hoping we
 * create sufficient numbers and they don't utilise too many resources. This
 * is a bad hack, but works for the time being...
 */
static void
gegl_expcombine_attach (GeglOperation *operation)
{
  GObjectClass  *object_class = G_OBJECT_GET_CLASS (operation);
  gchar padname[16];
  guint i;

  g_object_class_install_property (G_OBJECT_GET_CLASS (operation),
                                   PROP_INPUT,
                                   g_param_spec_object ("output",
                                                        "output",
                                                        "Output buffer",
                                                        GEGL_TYPE_BUFFER,
                                                        G_PARAM_READWRITE |
                                                        GEGL_PARAM_PAD_OUTPUT));
  gegl_operation_create_pad (operation,
                             g_object_class_find_property (object_class,
                                                           "output"));
  for (i = 0; i <= 99; ++i)
    {
      snprintf (padname, G_N_ELEMENTS (padname), "exposure_%u", i);
      g_object_class_install_property (G_OBJECT_GET_CLASS (operation),
                                       PROP_INPUT,
                                       g_param_spec_object (padname,
                                                            padname,
                                                            "Exposure input.",
                                                            GEGL_TYPE_BUFFER,
                                                            G_PARAM_READWRITE |
                                                            GEGL_PARAM_PAD_INPUT));
      gegl_operation_create_pad (operation,
                                 g_object_class_find_property (object_class,
                                                               padname));
    }
}



static void
gegl_expcombine_prepare (GeglOperation *operation)
{
  GSList *inputs = gegl_node_get_input_pads (operation->node);

  /* Set all the pads output formats, and ensure there is an appropriate node
   * property for each */
  for (; inputs; inputs = inputs->next)
    {
      GeglPad     *pad     = inputs->data;
      const gchar *padname = gegl_pad_get_name (pad);

      gegl_pad_set_format (pad, babl_format (PAD_FORMAT));

      /* Create properties for all pads that don't have them, allows arbitrary
       * numbers of pads.
       */
      if (g_object_class_find_property (G_OBJECT_GET_CLASS (operation),
                                        padname))
          continue;

      g_warning ("Could not find property for pad '%s'",
                 gegl_pad_get_name (pad));
    }

  gegl_operation_set_format (operation, "output", babl_format (PAD_FORMAT));
}


/* Order pads beginning with EXP_PREFIX by their trailing exposure value.
 * Non-exposure pads come first, then exposure pads without an EV, then
 * ordered exposure pads.
 */
static int
gegl_expcombine_pad_cmp (gconstpointer _a, gconstpointer _b)
{
  const gchar *a = gegl_pad_get_name (GEGL_PAD (_a)),
              *b = gegl_pad_get_name (GEGL_PAD (_b));
  guint64      x, y;

  if (!g_str_has_prefix (b, EXP_PREFIX)) return  1;
  if (!g_str_has_prefix (a, EXP_PREFIX)) return -1;

  a = strrchr (a, '-');
  b = strrchr (b, '-');

  g_return_val_if_fail (b, -1);
  g_return_val_if_fail (a, -1);

  y = g_ascii_strtoll (b + 1, NULL, 10);
  if (errno) return  1;
  x = g_ascii_strtoll (a + 1, NULL, 10);
  if (errno) return -1;

  if (x < y) return -1;
  if (x > y) return  1;
  return 0;
}


/* Extract a list of exposure pixel and metadata from the operation context.
 * The data is not guaranteed to be ordered within the list. May return NULL
 * for error.
 */
static GSList *
gegl_expcombine_get_exposures (GeglOperation        *operation,
                               GeglOperationContext *context,
                               const GeglRectangle  *full_roi,
                               GeglRectangle        *scaled_roi)
{
  GeglChantO    *o          = GEGL_CHANT_PROPERTIES (operation);
  gchar         *ev_cursor  = o->exposures;
  GSList        *exposures  = NULL,
                *inputs,
                *cursor;
  guint          components = babl_format_get_n_components (babl_format (PAD_FORMAT));
  gfloat         scale;

  g_return_val_if_fail (operation, NULL);
  g_return_val_if_fail (context, NULL);
  g_return_val_if_fail (full_roi, NULL);
  g_return_val_if_fail (!gegl_rectangle_is_empty (full_roi), NULL);
  g_return_val_if_fail (scaled_roi, NULL);

  /* Calculate the target dimensions for the downscaled buffer used in the
   * curve recovery. Does not enlarge the buffer.
   * sqrt does not cause issues with rectangular buffers as it's merely a
   * scaling factor for later.
   */
  {
    scale       = MAX_SCALED_PIXELS /
                  (gfloat)(full_roi->width * full_roi->height);
    scale       = sqrtf (scale);
    *scaled_roi = *full_roi;

    if (scale > 1.0f)
      {
        scale = 1.0f;
      }
    else
      {
        scaled_roi->width  = full_roi->width  * scale;
        scaled_roi->height = full_roi->height * scale;
      }
  }

  inputs = g_slist_copy (gegl_node_get_input_pads (operation->node));
  inputs = g_slist_sort (inputs, gegl_expcombine_pad_cmp);

  /* Read in each of the exposure buffers */
  for (cursor = inputs; cursor; cursor = cursor->next)
    {
      GeglBuffer *buffer;
      GeglPad    *pad = cursor->data;
      exposure   *e;

      /* Weed out non-exposure pads */
      if (!gegl_expcombine_is_exposure_pad (pad))
        {
          if (strcmp (gegl_pad_get_name (pad), "aux"))
              g_warning ("Unexpected pad name '%s' in exp-combine",
                         gegl_pad_get_name (pad));
          continue;
        }

      /* Add exposure to list */
      buffer  = gegl_operation_context_get_source (context,
                                                   gegl_pad_get_name (pad));
      if (!buffer)
          continue;

      e                = gegl_expcombine_new_exposure ();
      e->pixels[PIXELS_FULL]   = g_new (gfloat, full_roi->width  *
                                                full_roi->height *
                                                components);
      gegl_buffer_get (buffer, full_roi, 1.0, babl_format (PAD_FORMAT),
                       e->pixels[PIXELS_FULL], GEGL_AUTO_ROWSTRIDE,
                       GEGL_ABYSS_NONE);

      g_return_val_if_fail (scale <= 1.0f, NULL);
      if (scale == 1.0f)
          e->pixels[PIXELS_SCALED] = e->pixels[PIXELS_FULL];
      else
        {
          e->pixels[PIXELS_SCALED] = g_new (gfloat,
                                            (scaled_roi->width  *
                                             scaled_roi->height *
                                             components));
          gegl_buffer_get (buffer, scaled_roi, scale, babl_format (PAD_FORMAT),
                           e->pixels[PIXELS_SCALED], GEGL_AUTO_ROWSTRIDE,
                           GEGL_ABYSS_NONE);
        }

      e->pixels[PIXELS_ACTIVE] = e->pixels[PIXELS_FULL];

      /* Read the exposure time: relate APEX brightness value only as a
       * function of exposure time that is assume aperture = 1 and
       * sensitivity = 1
       */
      e->ti = g_ascii_strtod (ev_cursor, &ev_cursor);
      e->ti = 1.0f / powf (2.0f, e->ti);

      /* Krawczyk: absolute calibration: this magic multiplier is a result
       * of my research in internet plus a bit of tweaking with a luminance
       * meter. tested with canon 350d, so might be a bit off for other
       * cameras.
       *   e->ti /= 1.0592f * 11.4f / 3.125f;
       */

      /* For the sake of sanity, we scale the times to produce an image which
       * is much closer to the 'normal' 0-1 range. This prevents components
       * from going massively outside of our gamut.
       *
       * In particular this aids visual inspection and use of standard pixel
       * value comparison functions in testing.
       */
      e->ti *= 5e4f;

      if (errno)
        {
          g_warning ("Invalid exposure values specified for exp-combine");
          g_slist_foreach (exposures,
                           (GFunc)gegl_expcombine_destroy_exposure,
                           NULL);
          g_slist_free    (exposures);

          return NULL;
        }

      exposures = g_slist_prepend (exposures, e);
    }

  /* Link each exposure's high and low sibling pointers. Simplifies
   * anti-ghosting computations later.
   */
  exposures = g_slist_sort (exposures, gegl_expcombine_exposure_cmp);
  {
    exposure *prev = exposures->data;
    for (cursor = exposures; cursor; cursor = cursor->next)
      {
        exposure *e = cursor->data;

        prev->hi = e;
        e->lo    = prev;
        prev     = e;
      }
  }

  if (exposures == NULL)
      g_warning ("exp-combine did not find any suitable input pads");
  if (*ev_cursor != '\0')
      g_warning ("too many supplied EVs for input pads");

  g_slist_free (inputs);
  return exposures;
}


#ifdef DEBUG_SAVE_CURVES
static void
gegl_expcombine_save_curves (const gfloat *response,
                             guint         steps,
                             const gchar  *path)
{
  guint i;
  FILE *fd = fopen (path, "w");

  for (i = 0; i < steps; ++i)
    fprintf (fd, "%u %f\n", i, response[i]);
  fclose (fd);

  return;
}
#endif


static gboolean
gegl_expcombine_process (GeglOperation        *operation,
                         GeglOperationContext *context,
                         const gchar          *output_pad,
                         const GeglRectangle  *full_roi,
                         gint                  level)
{
  GeglChantO *o           = GEGL_CHANT_PROPERTIES (operation);
  GeglBuffer *output      = gegl_operation_context_get_target (context,
                                                               output_pad);

  GeglRectangle   scaled_roi;
  GSList         *cursor;
  GSList         *exposures   = gegl_expcombine_get_exposures (operation,
                                                               context,
                                                               full_roi,
                                                               &scaled_roi);

  guint   i;
  guint   steps      = 1 << o->steps,
          step_max   = 0,
          step_min   = steps - 1;

  guint   components = babl_format_get_n_components (babl_format (PAD_FORMAT));
  gfloat *hdr        = g_new (gfloat, full_roi->width  *
                                      full_roi->height *
                                      components);
  gfloat *weights     =   g_new (gfloat, steps);
  gfloat *response[3] = { g_new (gfloat, steps),
                          g_new (gfloat, steps),
                          g_new (gfloat, steps) };

  guint   saturated  = 0;
  gfloat  over       = 0.0f,
          under      = 0.0f;

  g_return_val_if_fail (output,    FALSE);
  g_return_val_if_fail (exposures, FALSE);

  g_return_val_if_fail (steps      > 0, FALSE);
  g_return_val_if_fail (components > 0, FALSE);

  /* Find the highest and lowest valid steps in the output. Remap from the
   * 'normal' 0.0-1.0 floats to the range of integer steps.
   */
  for (cursor = exposures; cursor; cursor = cursor->next)
    {
      exposure *e = cursor->data;

      for (i = 0; i < full_roi->width * full_roi->height * components; ++i)
        {
          /* Clamp the values we receive to [0.0, 1.0) and record the
           * magnitude of this over/underflow.
           */
          if (e->pixels[PIXELS_FULL][i] <= 0.0f)
            {
              under += fabs (e->pixels[PIXELS_FULL][i]);
              e->pixels[PIXELS_FULL][i] = 0.0f;
            }
          else if (e->pixels[PIXELS_FULL][i] > 1.0f)
            {
              over  += e->pixels[PIXELS_FULL][i] - 1.0f;
              e->pixels[PIXELS_FULL][i] = 1.0f;
            }

          e->pixels[PIXELS_FULL][i] *= (steps - 1);
          if (e->pixels[PIXELS_FULL][i] > 0.0f)
            {
              step_max = MAX (step_max, e->pixels[PIXELS_FULL][i]);
              step_min = MIN (step_min, e->pixels[PIXELS_FULL][i]);
            }
        }

      if (e->pixels[PIXELS_SCALED] == e->pixels[PIXELS_FULL])
          continue;

      for (i = 0; i < scaled_roi.width * scaled_roi.height * components; ++i)
        {
          e->pixels[PIXELS_SCALED][i]  = CLAMP (e->pixels[PIXELS_SCALED][i], 0.0f, 1.0f);
          e->pixels[PIXELS_SCALED][i] *= (steps - 1);
        }

    }

  g_return_val_if_fail (step_max >= step_min, FALSE);
  if (under || over)
      g_warning ("Unexpected pixel bounds. "
                 "Overflow error: %f, underflow error: %f",
                 over  / components,
                 under / components);

  /* Initialise response curves and weights for estimation. Estimate and
   * retrieve the converted output. Apply the response curves to the input
   * using the debevec model rather than using the robertson output.
   */
  gegl_expcombine_weights_gauss (weights, steps, step_min, step_max, o->sigma);
  gegl_expcombine_exposures_set_active (exposures, PIXELS_SCALED);

  for (i = 0; i < components; ++i)
    {
      gegl_expcombine_response_linear (response[i], steps);
      saturated += gegl_expcombine_get_response (hdr, i, components, exposures,
                                                 response[i], weights, steps,
                                                 &scaled_roi);
    }

#ifdef DEBUG_SAVE_CURVES
  gegl_expcombine_save_curves (response[0], steps, "response_r");
  gegl_expcombine_save_curves (response[1], steps, "response_g");
  gegl_expcombine_save_curves (response[2], steps, "response_b");
#endif

  gegl_expcombine_exposures_set_active (exposures, PIXELS_FULL);
  gegl_expcombine_apply_debevec (hdr, components, exposures, response, weights,
                            steps, full_roi);

  g_return_val_if_fail (G_N_ELEMENTS (response) == 3, FALSE);
  g_free (response[0]);
  g_free (response[1]);
  g_free (response[2]);
  g_free (weights);

  if (saturated)
    {
      GEGL_NOTE (GEGL_DEBUG_PROCESS,
                 "exp-combine discovered %u saturated pixels (%f%%), "
                 "results may be suboptimal", saturated,
                 100.0f * saturated / full_roi->width / full_roi->height);
    }

  /* Rescale the output so we can more easily visually inspect the output
   * using LDR outputs.
   */
#ifdef DEBUG_LINEARIZE
  {
    gfloat max = G_MINFLOAT;
    guint  i;
    for (i = 0; i < full_roi->width * full_roi->height * components; ++i)
        max = MAX (max, hdr[i]);

    for (i = 0; i < full_roi->width * full_roi->height * components; ++i)
        hdr[i] /= max;
  }
#endif

  /* Save the HDR components to the output buffer. */
  gegl_buffer_set (output, full_roi, 0, babl_format (PAD_FORMAT), hdr,
                   GEGL_AUTO_ROWSTRIDE);
  gegl_cache_computed (gegl_node_get_cache (operation->node), full_roi);

  /* Cleanup */
  g_free (hdr);

  g_slist_foreach (exposures, (GFunc)gegl_expcombine_destroy_exposure, NULL);
  g_slist_free    (exposures);

  return TRUE;
}


/* Grab each source's dimensions, and update a running bounding box with the
 * union of itself and the current source.
 * TODO: properly support inputs of varying dimensions
 */
static GeglRectangle
gegl_expcombine_get_bounding_box (GeglOperation *operation)
{
  GeglRectangle  result = {0,0,0,0};
  GSList        *inputs = gegl_node_get_input_pads (operation->node);

  for (; inputs; inputs = inputs->next)
    {
      GeglPad             *pad = inputs->data;
      const GeglRectangle *newrect;

      if (!gegl_expcombine_is_exposure_pad (pad))
        continue;

      /* Get the source bounds and update with the union */
      newrect = gegl_operation_source_get_bounding_box (operation,
                                                        gegl_pad_get_name (pad));
      if (!newrect)
        continue;

      if (!gegl_rectangle_is_empty (&result) &&
          !gegl_rectangle_equal (newrect, &result))
        {
          g_warning ("expcombine inputs are of varying dimensions");
        }
      gegl_rectangle_bounding_box (&result, newrect, &result);
    }

  if (gegl_rectangle_is_empty (&result))
      g_warning ("Bounding box for exp-combine should not be empty");
  return result;
}


static GeglRectangle
gegl_expcombine_get_cached_region (GeglOperation       *operation,
                                   const GeglRectangle *roi)
{
  return gegl_expcombine_get_bounding_box (operation);
}


/* We have to use all the input to produce any output */
static GeglRectangle
gegl_expcombine_get_required_for_output (GeglOperation       *operation,
                                         const gchar         *input_pad,
                                         const GeglRectangle *output_roi)
{
  return gegl_expcombine_get_bounding_box (operation);
}

static void
gegl_chant_class_init (GeglChantClass *klass)
{
  GeglOperationClass       *operation_class;

  operation_class = GEGL_OPERATION_CLASS (klass);

  operation_class->attach            = gegl_expcombine_attach;
  operation_class->process           = gegl_expcombine_process;
  operation_class->get_bounding_box  = gegl_expcombine_get_bounding_box;
  operation_class->get_cached_region = gegl_expcombine_get_cached_region;

  operation_class->prepare     = gegl_expcombine_prepare;
  operation_class->get_required_for_output = gegl_expcombine_get_required_for_output;

  gegl_operation_class_set_keys (operation_class,
  "name"       , "gegl:exp-combine",
  "categories" , "compositors",
  "description",
      _("Combine multiple scene exposures into one high range buffer"),
      NULL);
}


#endif

