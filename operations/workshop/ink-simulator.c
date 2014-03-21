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
 * Copyright 2014 Øyvind Kolås <pippin@gimp.org>
 */

#include "config.h"
#include <glib/gi18n-lib.h>

#ifdef GEGL_CHANT_PROPERTIES

#define DEFAULT_CONFIG \
"illuminant=D65\n" \
"substrate=white\n" \
"\n"\
"ink1=cyan\n"\
"ink2=magenta\n"\
"ink3=yellow\n"\
"ink4=black\n"\
"\n"\
"inklimit=3.0\n"

gegl_chant_register_enum (ink_sim_mode)
  enum_value (GEGL_INK_SIMULATOR_PROOF, "proof")
  enum_value (GEGL_INK_SIMULATOR_SEPARATE, "separate")
  enum_value (GEGL_INK_SIMULATOR_SEPARATE_PROOF, "separate-proof")
gegl_chant_register_enum_end (GeglInkSimMode)

gegl_chant_multiline (config, _("ink configuration"), DEFAULT_CONFIG,
         _("Textual desciption of inks used for simulated print-job"))
gegl_chant_enum (mode, _("mode"), GeglInkSimMode, ink_sim_mode,
                 GEGL_INK_SIMULATOR_SEPARATE_PROOF, _("how the ink simulator is used"))

#else

#define GEGL_CHANT_TYPE_POINT_FILTER
#define GEGL_CHANT_C_FILE       "ink-simulator.c"

#include "gegl-chant.h"
#include <math.h>

#define SPECTRUM_BANDS 20
#define LUT_DIM        32
#define LUT_ITERATIONS 256
#define MAX_INKS       7

typedef struct _Config Config;
typedef struct _Ink Ink;
typedef struct _Spectrum Spectrum;

struct _Spectrum {
  gfloat bands[SPECTRUM_BANDS];
  /* 20 bands, spaced as follows :
   *
   * 0 = 390 - 409nm
   * 1 = 410 - 429nm
   * ....
   */
};

struct _Ink {
  Spectrum transmittance; /* spectral energy of this ink; as reflected of a fully reflective background */
  gfloat    opacity;  /* how much this ink covers up instead of mixes with background (substrate+inks) */
  gfloat    scale;   /* scale factor; increasing the amount of spectral contribution */

  /* per ink inklimit as well? */
};

typedef struct _InkMix InkMix;
struct _InkMix {
  gint   defined;
  gfloat level[MAX_INKS];
};

struct _Config {
  Spectrum illuminant;
  Spectrum substrate;
  Ink      ink_def[MAX_INKS];
  gint     inks;
  gfloat   ink_limit;
  InkMix   lut[LUT_DIM*LUT_DIM*LUT_DIM]; 
  gint     debug_width;
  gchar   *src;
};


static inline gint
lut_indice (gfloat  val,
            gfloat *delta)
{
  /* LUT_DIM-1 to have both 0.0 and 1.0 values to interpolate from */
  gint v = floor (val * (LUT_DIM - 1));
  if (v < 0) v = 0; if (v >= (LUT_DIM-1)) v = LUT_DIM - 2;
  if (delta)
    {
       *delta = ((val * (LUT_DIM - 1)) - v);
    }
  return v;
}

static inline gint
lut_index (gint ri,
           gint gi,
           gint bi)
{
  return ri * LUT_DIM * LUT_DIM + gi * LUT_DIM + bi;
}

/* from http://cvrl.ucl.ac.uk/cones.htm */
static Spectrum STANDARD_OBSERVER_L   = {{
0.002177, 0.0158711, 0.03687185, 0.06265, 0.13059, 0.26828, 0.57836, 0.849104, 0.9704655, 0.9793335, 0.85568175, 0.59266275, 0.30191075, 0.11285, 0.032197, 0.007657, 0.0017424, 0.0004039, 0.000009, 0.000002
}};
static Spectrum STANDARD_OBSERVER_M   = {{
0.00207, 0.01863, 0.057983, 0.11151, 0.221277, 0.4010105, 0.75920, 0.97423, 0.93118, 0.694917, 0.37578, 0.14093, 0.03995, 0.0096, 0.002, 0.0004, 0.0001095,
  0.0000026, 0
}};
static Spectrum STANDARD_OBSERVER_S   = {{
  0.05312, 0.45811575, 0.9226575, 0.83515125, 0.46085625, 0.146032125, 0.03805695, 0.00723844, 0.0010894, 0.000158823, 0.0000025, 0.000000336, 0
}};

#define TRANSMITTANCE_CYAN    {{0.8,0.8,0.9,0.95,0.95,0.95,0.9,0.9,0.82,0.75,0.6,0.55,0.5,0.4,0.3,0.22,0.3,0.37,0.57,0.72}}
#define TRANSMITTANCE_MAGENTA {{1,1,1,0,0,0,0,0,0,1,1,1,0,0,0,0,0,0,0,0}}
#define TRANSMITTANCE_YELLOW  {{0.42,0.48,0.53,0.58,0.64,0.68,0.77,0.86,0.9, 1,1,1,1,0,0,0,0,0,0,0}}

// this transmittance looks more blue than magenta:
//#define TRANSMITTANCE_MAGENTA {{1.0,0.7,0.9,0.85,0.8,0.7,0.64,0.54,0.5,0.38,0.6,0.8,0.9,1.0,1.0,1.0,1.0,0.9,0.9,0.9}}

#define TRANSMITTANCE_RED     {{0,0,0,0,0,0,0,0,0,0,1,1,1,1,1,1,1,0,0,0}}
#define TRANSMITTANCE_GREEN   {{0,0,0,0,0,1,1,1,1,0,0,0,0,0,0,0,0,0,0,0}}
#define TRANSMITTANCE_BLUE    {{0,1,1,1,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0}}
#define TRANSMITTANCE_BLACK   {{0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0}}
#define TRANSMITTANCE_GRAY    {{0.5,0.5,0.5,0.5,0.5,0.5,0.5,0.5,0.5,0.5,0.5,0.5,0.5,0.5,0.5,0.5,0.5,0.5,0.5,0.5}}
#define TRANSMITTANCE_WHITE   {{1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1}}

/* the even illuminant; which is easy and almost cheating */
#define ILLUMINANT_E         TRANSMITTANCE_WHITE

#define ILLUMINANT_D65 {{\
0.54648200, 0.91486000, 0.86682300, 1.17008000, 1.14861000, 1.08811000, 1.07802000, 1.07689000, 1.04046000, 0.96334200, 0.88685600, 0.89599100, 0.83288600, 0.80026800, 0.82277800, 0.69721300, 0.74349000, 0.69885600, 0.63592700, 0.66805400 }} 

#define ILLUMINANT_D55 {{\
0.3809, 0.6855, 0.6791, 0.9799, 0.9991, 0.9808, 1.0070, 1.0421, 1.0297, 0.9722, 0.9143, 0.9514, 0.9045, 0.8885, 0.9395, 0.7968, 0.8484, 0.7930, 0.7188, 0.7593 }}

#define ILLUMINANT_D50 {{\
0.4980, 0.5850, 0.5780, 0.8720, 0.9140, 0.9200, 0.9660, 1.0210, 1.0230, 0.9770, 0.9350, 0.9930, 0.9570, 0.9570, 1.0300, 0.8740, 0.9290, 0.8660, 0.7820, 0.8290 }}


static inline void
spectrum_remove_light (Spectrum       *s,
                       const Spectrum *ink,
                       gfloat          coverage)
{
  gint i;
  for (i = 0; i < SPECTRUM_BANDS; i++)
    s->bands[i] = s->bands[i] * (1.0 - coverage) +
                  s->bands[i] * ink->bands[i] * coverage;
}

static inline void
spectrum_add_opaque_ink (Spectrum       *s,
                         const Spectrum *ink,
                         const Spectrum *illuminant,
                         gfloat          coverage)
{
  gint i;
  for (i = 0; i < SPECTRUM_BANDS; i++)
    s->bands[i] = s->bands[i] * (1.0 - coverage) +
                  ink->bands[i] * illuminant->bands[i] * coverage;
}

static inline void
spectrum_lerp (Spectrum       *s,
               const Spectrum *a,
               const Spectrum *b,
               gfloat          delta)
{
  gint i;
  for (i = 0; i < SPECTRUM_BANDS; i++)
    s->bands[i] = a->bands[i] * (1.0 - delta) + b->bands[i] * delta;
}

static inline void
add_ink (Spectrum       *s,
         const Spectrum *illuminant,
         const Spectrum *ink,
         gfloat          coverage,
         gfloat          opacity)
{
  if (fabs (opacity - 1.0) < 0.01)
  {
    spectrum_add_opaque_ink (s, ink, illuminant, coverage);
  }
  else if (fabs (opacity) < 0.01)
  {
    spectrum_remove_light (s, ink, coverage);
  }
  else
  {
    Spectrum opaque = *s;
    spectrum_add_opaque_ink (&opaque, ink, illuminant, coverage);
    spectrum_remove_light (s, ink, coverage);
  
    /* linear interpolation of the two simpler cases */

    spectrum_lerp (s, s, &opaque, opacity);
  }
}

static gfloat spectrum_integrate (const Spectrum *s,
                                  const Spectrum *is,
                                  gfloat          scale)
{
  gfloat result = 0.0;
  gint i;
  for (i = 0; i < SPECTRUM_BANDS; i++)
    result += s->bands[i] * is->bands[i];
  return (result / SPECTRUM_BANDS) * scale;
}

static void parse_config (GeglOperation *operation);

static void
prepare (GeglOperation *operation)
{
  /* this RGBA float is interpreted as CMYK ! */
  gegl_operation_set_format (operation, "input", babl_format ("RGBA float"));
  gegl_operation_set_format (operation, "output", babl_format ("RGBA float"));

  parse_config (operation);
}

static inline void
spectrum_to_rgba (const Spectrum *spec,
                  gfloat         *rgba)
{
  gfloat l, m, s;
  l = spectrum_integrate (spec, &STANDARD_OBSERVER_L, 0.5);
  m = spectrum_integrate (spec, &STANDARD_OBSERVER_M, 0.5);
  s = spectrum_integrate (spec, &STANDARD_OBSERVER_S, 0.5);
  /* this LMS to linear RGB -- likely not right..  */
  rgba[0] = ((30.83) * l + (-29.83) * m + (1.61) * s);
  rgba[1] = ((-6.481) * l + (17.7155) * m + (-2.532) * s);
  rgba[2] = ((-0.375) * l + (-1.19906) * m + (14.27) * s);
  rgba[3] = 1.0;
}

static inline Spectrum
inks_to_spectrum (Config *config,
                  gfloat *ink_levels)
{
  gint i;
  Spectrum spec = config->illuminant;
  spectrum_remove_light (&spec, &config->substrate, 1.0);

  for (i = 0; i < MIN(4, config->inks); i++) /* XXX: MIN(4, to avoid walking out of RGBA */
    add_ink (&spec, &config->illuminant,
                    &config->ink_def[i].transmittance,
                    ink_levels[i] * config->ink_def[i].scale,
                    config->ink_def[i].opacity);
  return spec;
}

static inline void
spectral_proof (Config *config,
                gfloat *ink_levels,
                gfloat *rgba)
{
  Spectrum spec = inks_to_spectrum (config, ink_levels);
  spectrum_to_rgba (&spec, rgba);
}

static inline gfloat
colordiff (gfloat *rgb_a,
           gfloat *rgb_b)
{
  return 
    (rgb_a[0]-rgb_b[0])*(rgb_a[0]-rgb_b[0])+
    (rgb_a[1]-rgb_b[1])*(rgb_a[1]-rgb_b[1])+
    (rgb_a[2]-rgb_b[2])*(rgb_a[2]-rgb_b[2]);
}

static inline void
rgb_to_inks_stochastic (Config *config,
                        gfloat  *rgb,
                        gfloat  *ink_levels,
                        gint    iterations,
                        gfloat  rrange)
{
  gfloat best[MAX_INKS] = {};
  gfloat bestdiff = 1000.0;
  gint i;
  for (i = 0; i < config->inks; i++)
    best[i] = 0.0; /* XXX: maybe 0.0 is better to limit inks,
                           and metamers? */

  for (i = 0; i < iterations; i++)
  {
    gint j;
    gfloat attempt[8];
    gfloat softrgb[4];
    gfloat diff;
    gfloat inksum = 0;

    do {
      inksum = 0.0;
      for (j = 0; j < config->inks; j++)
      {
        attempt[j] = best[j] + g_random_double_range(-rrange, rrange);
        attempt[j] = CLAMP(attempt[j],0,1);
        inksum += attempt[j];
      }
    } while (inksum > config->ink_limit);

    spectral_proof (config, attempt, softrgb);
    diff = colordiff (rgb, softrgb) + (inksum / 50.0) * (inksum / 50.0);
    if (diff < bestdiff)
    {
      rrange *= 0.99;
      bestdiff = diff;
      for (j = 0; j < config->inks; j++)
        best[j] = attempt[j];
      if (diff < 0.0001) /* close enough */
        break;
    }
  }

  for (i = 0; i < config->inks; i++)
    ink_levels[i] = best[i];
}

static inline gfloat *
ensure_lut (Config *config,
            gint    ri,
            gint    gi,
            gint    bi)
{
  gint l_index;
  l_index = lut_index (ri, gi, bi);
  if (!config->lut[l_index].defined)
  {
    gfloat trgb[3] = {(float)ri / LUT_DIM,
                      (float)gi / LUT_DIM,
                      (float)bi / LUT_DIM };

    rgb_to_inks_stochastic (config, trgb, &config->lut[l_index].level[0], LUT_ITERATIONS, 0.6);
    config->lut[l_index].defined = 1;
  }
  return &config->lut[l_index].level[0];
}

static inline void
lerp_inks (gint          inks,
           gfloat       *ink_res,
           const gfloat *inka,
           const gfloat *inkb,
           gfloat        delta)
{
  gint i;
  for (i = 0; i < inks; i++)
    ink_res[i] = inka[i]  * (1.0 - delta) + inkb[i] * delta;
}

static inline void rgb_to_inks (Config *config,
                                gfloat *rgb,
                                gfloat *ink_levels)
{
  gfloat rdelta, gdelta, bdelta;
  gint ri = lut_indice (rgb[0], &rdelta);
  gint gi = lut_indice (rgb[1], &gdelta);
  gint bi = lut_indice (rgb[2], &bdelta);
  gfloat *ink_corner[8];
  gfloat  temp1[MAX_INKS];
  gfloat  temp2[MAX_INKS];
  gfloat  temp3[MAX_INKS];
  gfloat  temp4[MAX_INKS];

/* numbering of corners, and positions of R,G,B axes
      6
      /\
    /   \
 7/   d  \5
  |\     /|
  |  \4/  |
 3\B G|2  |
   \  |  /1
     \|/R
      0       */

  ink_corner[0] = ensure_lut (config, ri + 0, gi + 0, bi + 0);
  ink_corner[1] = ensure_lut (config, ri + 1, gi + 0, bi + 0);
  ink_corner[2] = ensure_lut (config, ri + 1, gi + 0, bi + 1);
  ink_corner[3] = ensure_lut (config, ri + 0, gi + 0, bi + 1);

  ink_corner[4] = ensure_lut (config, ri + 0, gi + 1, bi + 0);
  ink_corner[5] = ensure_lut (config, ri + 1, gi + 1, bi + 0);
  ink_corner[6] = ensure_lut (config, ri + 1, gi + 1, bi + 1);
  ink_corner[7] = ensure_lut (config, ri + 0, gi + 1, bi + 1);

  lerp_inks (config->inks, temp1, ink_corner[0], ink_corner[1], rdelta);
  lerp_inks (config->inks, temp2, ink_corner[3], ink_corner[2], rdelta);
  lerp_inks (config->inks, temp3, ink_corner[4], ink_corner[5], rdelta);
  lerp_inks (config->inks, temp4, ink_corner[7], ink_corner[6], rdelta);
  lerp_inks (config->inks, temp1, temp1, temp3, gdelta);
  lerp_inks (config->inks, temp2, temp2, temp4, gdelta);
  lerp_inks (config->inks, ink_levels, temp1, temp2, bdelta);
}

static gboolean
process (GeglOperation       *op,
         void                *in_buf,
         void                *out_buf,
         glong                samples,
         const GeglRectangle *roi,
         gint                 level)
{
  GeglChantO *o = GEGL_CHANT_PROPERTIES (op);
  gfloat *in  = in_buf;
  gfloat *out = out_buf;
  Config *config = o->chant_data;

  switch (o->mode)
  {
    case GEGL_INK_SIMULATOR_PROOF:
      {
        while (samples--)
          {
            spectral_proof  (config, in, out);
            in  += 4;
            out += 4;
          }
      }
      break;
    case GEGL_INK_SIMULATOR_SEPARATE:
    while (samples--)
      {
        int i;
        gfloat inks[MAX_INKS];
        rgb_to_inks (config, in, inks);
        for (i = 0; i < MIN(4, config->inks); i++)
          out[i] = inks[i];
        if (config->inks < 4)
          out[3] = 1.0;
        if (config->inks < 3)
          out[2] = 0.0;
        if (config->inks < 2)
          out[1] = 0.0;
        in  += 4;
        out += 4;
      }
      break;
    case GEGL_INK_SIMULATOR_SEPARATE_PROOF:
      if (config->debug_width)
      {
        gint x = roi->x;
        gint y = roi->y;
        while (samples--)
          {
            gfloat inks[MAX_INKS];
            gint foo = ((x+y)/config->debug_width);
            gint actual_inks = config->inks;
            rgb_to_inks (config, in, inks);
            config->inks = MIN(foo, actual_inks);
            spectral_proof (config, inks, out);
            config->inks = actual_inks;

            in  += 4;
            out += 4;

            x++; if (x>=roi->x+roi->width){x=roi->x;y++;}
          }
      }
      else
      {
        while (samples--)
          {
            gfloat inks[MAX_INKS];
            rgb_to_inks (config, in, inks);
            spectral_proof (config, inks, out);

            in  += 4;
            out += 4;
          }
      }
      break;
  }

  return TRUE;
}

static Spectrum parse_spectrum (gchar *spectrum)
{
  Spectrum s = TRANSMITTANCE_WHITE;
  if (!spectrum)
    return s;
  while (*spectrum == ' ') spectrum ++;

  if (g_str_has_prefix (spectrum, "rgb"))
  {
    Spectrum red = TRANSMITTANCE_RED;
    Spectrum green = TRANSMITTANCE_GREEN;
    Spectrum blue = TRANSMITTANCE_BLUE;
    gint i;
    gfloat r = 0, g = 0, b = 0;
    gchar *p = spectrum + 3;

    r = g_strtod (p, &p);
    if (p) g = g_strtod (p, &p);
    if (p) b = g_strtod (p, &p);

    for (i = 0; i<SPECTRUM_BANDS; i++)
      s.bands[i] = red.bands[i] * r + green.bands[i] * g + blue.bands[i] * b;

  } else if (g_str_has_prefix (spectrum, "red"))
  { Spectrum color = TRANSMITTANCE_RED; s = color;
  } else if (g_str_has_prefix (spectrum, "green"))
  { Spectrum color = TRANSMITTANCE_GREEN; s = color;
  } else if (g_str_has_prefix (spectrum, "blue"))
  { Spectrum color = TRANSMITTANCE_BLUE; s = color;
  } else if (g_str_has_prefix (spectrum, "cyan"))
  { Spectrum color = TRANSMITTANCE_CYAN; s = color;
  } else if (g_str_has_prefix (spectrum, "yellow"))
  { Spectrum color = TRANSMITTANCE_YELLOW; s = color;
  } else if (g_str_has_prefix (spectrum, "magenta"))
  { Spectrum color = TRANSMITTANCE_MAGENTA; s = color;
  } else if (g_str_has_prefix (spectrum, "white"))
  { Spectrum color = TRANSMITTANCE_WHITE; s = color;
  } else if (g_str_has_prefix (spectrum, "D50"))
  { Spectrum color = ILLUMINANT_D50; s = color;
  } else if (g_str_has_prefix (spectrum, "D55"))
  { Spectrum color = ILLUMINANT_D55; s = color;
  } else if (g_str_has_prefix (spectrum, "D65"))
  { Spectrum color = ILLUMINANT_D65; s = color;
  } else if (g_str_has_prefix (spectrum, "E"))
  { Spectrum color = ILLUMINANT_E; s = color;
  } else if (g_str_has_prefix (spectrum, "black"))
  { Spectrum color = TRANSMITTANCE_BLACK; s = color;
  } else if (g_str_has_prefix (spectrum, "gray"))
  { Spectrum color = TRANSMITTANCE_GRAY; s = color;
  } else
  {
    gint band = 0;
    band = 0;
    do {
    s.bands[band++] = g_strtod (spectrum, &spectrum);
    } while (spectrum && band < SPECTRUM_BANDS);
  }

  return s;
}

static void parse_config_line (GeglOperation *operation,
                               Config        *config,
                               const gchar   *line)
{
  Spectrum s;
  gint i;
  if (!line)
    return;
  if (!strchr (line, '='))
    return;
  while (*line == ' ') line++;
  s = parse_spectrum (strchr (line, '=') +1);
  if (g_str_has_prefix (line, "illuminant"))
    config->illuminant = s;
  else if (g_str_has_prefix (line, "substrate"))
    config->substrate = s;
  else if (g_str_has_prefix (line, "inklimit"))
    {
      config->ink_limit = strchr(line, '=') ? g_strtod (strchr (line, '=')+1, NULL) : 3.0;
      if (config->ink_limit < 0.2)
        config->ink_limit = 0.2;
    }
  else if (g_str_has_prefix (line, "debugwidth"))
    {
      config->debug_width= strchr(line, '=') ? g_strtod (strchr (line, '=')+1, NULL) : 25;
    }
  else
  for (i = 0; i < MAX_INKS; i++)
  {
    gchar prefix[] = "ink1";
    prefix[3] = (i+1) + '0';
    if (g_str_has_prefix (line, prefix))
      {
        config->ink_def[i].transmittance = s;
        if (strstr(line, "o=")) config->ink_def[i].opacity = g_strtod (strstr(line, "o=") + 2, NULL);
        if (strstr(line, "s=")) config->ink_def[i].scale = g_strtod (strstr(line, "s=") + 2, NULL);
        config->inks = MAX(config->inks, i+1);
      }
  }
}

static void parse_config (GeglOperation *operation)
{
  GeglChantO *o = GEGL_CHANT_PROPERTIES (operation);
  const char *p = o->config;
  GString *str;
  Config *config = o->chant_data;
  
  gint i;
  if (!p)
    return;

  if (!config)
  {
    config = g_new0 (Config, 1);
    o->chant_data = config;
  }
  
  if (config->src)
    {
      if (!strcmp (config->src, p))
        return;
      g_free (config->src);
    }

  memset (config, 0, sizeof (Config));
  for (i = 0; i < MAX_INKS; i++) config->ink_def[i].scale = 1.0;
  config->ink_limit = MAX_INKS;

  str = g_string_new ("");
  while (*p)
  {
    switch (*p)
    {
      case '\n':
        parse_config_line (operation, config, str->str);
        g_string_assign (str, "");
        break;
      default:
        g_string_append_c (str, *p);
        break;
    }
    p++;
  }

  g_string_free (str, TRUE);
  config->src = g_strdup (o->config);
}


static void
finalize (GObject *object)
{
  GeglChantO *o = GEGL_CHANT_PROPERTIES (object);
  if (o->chant_data)
  {
    g_free (o->chant_data);
    o->chant_data = NULL;
  }
  G_OBJECT_CLASS (g_type_class_peek_parent (G_OBJECT_GET_CLASS (object)))->finalize (object);
}

static void
gegl_chant_class_init (GeglChantClass *klass)
{
  GeglOperationClass            *operation_class;
  GeglOperationPointFilterClass *point_filter_class;
  GObjectClass                  *gobject_class;
  
  operation_class = GEGL_OPERATION_CLASS (klass);
  point_filter_class = GEGL_OPERATION_POINT_FILTER_CLASS (klass);
  gobject_class = G_OBJECT_CLASS (klass);

  gobject_class->finalize = finalize;
  point_filter_class->process = process;
  operation_class->prepare = prepare;

  gegl_operation_class_set_keys (operation_class,
  "name"        , "gegl:ink-simulator",
  "categories"  , "misc",
  "description" ,
        _("spectral ink simulator, for softproofing/simulating press inks"
          ""),
        NULL);
}

#endif
