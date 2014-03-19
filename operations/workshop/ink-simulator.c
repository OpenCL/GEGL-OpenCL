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

#define DEFAULT_INKS \
"illuminant=1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1\n" \
"substrate= rgb 0.98 0.98 0.98 \n" \
"ink1=cyan    s=1.5\n"\
"ink2=magenta s=1.5\n"\
"ink3=yellow  s=1.5\n"\
"ink4=black   s=1\n"\
"inklimit=150\n"

gegl_chant_register_enum (ink_sim_mode)
  enum_value (GEGL_INK_SIMULATOR_PROOF, "simulate printing of inks")
  enum_value (GEGL_INK_SIMULATOR_SEPARATE, "separate to inks")
  enum_value (GEGL_INK_SIMULATOR_SEPARATE_PROOF, "separate and simulate")
gegl_chant_register_enum_end (GeglInkSimMode)

gegl_chant_multiline (config, _("ink configuration"), DEFAULT_INKS,
         _("Textual desciption of inks passed in"))

gegl_chant_enum (mode, _("mode"), GeglInkSimMode, ink_sim_mode, GEGL_INK_SIMULATOR_SEPARATE_PROOF, _("how the ink simulator is used"))

#else

#define GEGL_CHANT_TYPE_POINT_FILTER
#define GEGL_CHANT_C_FILE       "ink-simulator.c"

#include "gegl-chant.h"

#define SPECTRUM_BANDS 20

typedef struct _Config Config;
typedef struct _Ink Ink;
typedef struct _Spectrum Spectrum;


struct _Spectrum {
  float bands[SPECTRUM_BANDS];
  /* 20 bands, spaced as follows :
   *
   * 0 = 390 - 405nm
   * 1 = 410 - 425nm
   * ....
   */
};

struct _Ink {
  Spectrum transmittance; /* spectral energy of this ink; as reflected of a fully reflective background */
  float    opacity;       /* how much this ink covers up instead of mixes with background (substrate+inks) */
  float    scale;         /* scale factor; increasing the amount of spectral contribution */
};

struct _Config {
  Spectrum illuminant;
  Spectrum substrate;
  Ink      ink_def[4];
  float    ink_limit;
  int      inks;
};

static Config config;

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

// this tranmittans looks more blue than magenta:
//#define TRANSMITTANCE_MAGENTA {{1.0,0.7,0.9,0.85,0.8,0.7,0.64,0.54,0.5,0.38,0.6,0.8,0.9,1.0,1.0,1.0,1.0,0.9,0.9,0.9}}

#define TRANSMITTANCE_RED     {{0,0,0,0,0,0,0,0,0,0,1,1,1,1,1,1,1,0,0,0}}
#define TRANSMITTANCE_GREEN   {{0,0,0,0,0,1,1,1,1,0,0,0,0,0,0,0,0,0,0,0}}
#define TRANSMITTANCE_BLUE    {{0,1,1,1,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0}}
#define TRANSMITTANCE_BLACK   {{0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0}}
#define TRANSMITTANCE_GRAY    {{0.5,0.5,0.5,0.5,0.5,0.5,0.5,0.5,0.5,0.5,0.5,0.5,0.5,0.5,0.5,0.5,0.5,0.5,0.5,0.5}}
#define TRANSMITTANCE_WHITE   {{0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1}}
#define ILLUMINANT_E         TRANSMITTANCE_WHITE

/* the even illuminant; which is easy and almost cheating */

//static int inks = 3;
//static Ink ink_defs[4] = {{{{0}}}};
//static Spectrum ILLUMINANT              = ILLUMINANT_E;
//static Spectrum SUBSTRATE_TRANSMITTANCE = TRANSMITTANCE_WHITE;

static inline Spectrum add_ink (Spectrum s, Spectrum ink, float coverage, float opacity)
{
  int i;
  for (i = 0; i < SPECTRUM_BANDS; i++)
  {
    float transparent = s.bands[i] * (1.0 - coverage) + s.bands[i] * ink.bands[i] * coverage;
    float opaque      = s.bands[i] * (1.0 - coverage) + ink.bands[i] * coverage;
    s.bands[i] = opaque * opacity + transparent * (1.0 - opacity);
  }
  return s;
}

static Spectrum spectrum_remove_light (Spectrum s, Spectrum ink, float coverage)
{
  int i;
  for (i = 0; i < SPECTRUM_BANDS; i++)
  {
    s.bands[i] = s.bands[i] * (1.0 - coverage) + s.bands[i] * ink.bands[i] * coverage;
  }
  return s;
}

static float spectrum_integrate (Spectrum s, Spectrum is, float scale)
{
  float result = 0.0;
  int i;
  for (i = 0; i < SPECTRUM_BANDS; i++)
    result += s.bands[i] * is.bands[i];
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

static inline void spectrum_to_rgba (Spectrum spec, float *rgba)
{
  float l, m, s;
  l = spectrum_integrate (spec, STANDARD_OBSERVER_L, 0.4);
  m = spectrum_integrate (spec, STANDARD_OBSERVER_M, 0.4);
  s = spectrum_integrate (spec, STANDARD_OBSERVER_S, 0.4);
  /* this LMS to linear RGB -- likely not right..  */
  rgba[0] = ((30.83) * l + (-29.83) * m + (1.61) * s);
  rgba[1] = ((-6.481) * l + (17.7155) * m + (-2.532) * s);
  rgba[2] = ((-0.375) * l + (-1.19906) * m + (14.27) * s);
  rgba[3] = 1.0;
}

static inline Spectrum inks_to_spectrum (Config *config, float *ink_levels)
{
  int i;
  Spectrum spec = config->illuminant;

  spec = spectrum_remove_light (spec, config->substrate, 1.0);
  for (i = 0; i < config->inks; i++)
    spec = add_ink (spec,
                    config->ink_def[i].transmittance,
                    ink_levels[i] * config->ink_def[i].scale,
                    config->ink_def[i].opacity);
  return spec;
}

static inline void spectral_proof (Config *config, float *ink_levels, float *rgba)
{
  spectrum_to_rgba (inks_to_spectrum (config, ink_levels), rgba);
}

static inline double colordiff (float *cola, float *colb)
{
  return 
    (cola[0]-colb[0])*(cola[0]-colb[0])+
    (cola[1]-colb[1])*(cola[1]-colb[1])+
    (cola[2]-colb[2])*(cola[2]-colb[2]);
}

static inline void rgb_to_inks (Config *config, float *rgb, float *ink_levels)
{
  float best[8] = {0.5,0.5,0.5,0.5,0.5};
  float bestdiff = 3;
  int i;

  float rrange = 0.5;
  for (i = 0; i < 64; i++)
  {
    int j;
    float attempt[8];
    float softrgb[4];
    float diff;
    float inksum = 0;
    do{
      inksum = 0.0;
      for (j = 0; j < config->inks; j++)
      {
        attempt[j] = best[j] + g_random_double_range(-rrange, rrange);
        if (attempt[j] < 0) attempt[j] = 0;
        if (attempt[j] > 1) attempt[j] = 1;
        inksum += attempt[j];
      }
    } while (inksum > config->ink_limit);

    spectral_proof (config, attempt, softrgb);
    diff = colordiff (rgb, softrgb);
    if (diff < bestdiff)
    {
      bestdiff = diff;
      for (j = 0; j < config->inks; j++)
        best[j] = attempt[j];
      rrange *= 0.8;
      //if (diff < 0.001) /* bail early */
      //  i = 1000;
    }
  }
  for (i = 0; i < config->inks; i++)
    ink_levels[i] = best[i];
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

  switch (o->mode)
  {
    case GEGL_INK_SIMULATOR_PROOF:
      while (samples--)
        {
          spectral_proof    (&config, in, out);
          in  += 4;
          out += 4;
        }
      break;
    case GEGL_INK_SIMULATOR_SEPARATE:
    while (samples--)
      {
        rgb_to_inks (&config, in, out);
        if (config.inks < 4)
          out[3] = 1.0;
        if (config.inks < 3)
          out[2] = 0.0;
        if (config.inks < 2)
          out[1] = 0.0;
        in  += 4;
        out += 4;
      }
      break;
    case GEGL_INK_SIMULATOR_SEPARATE_PROOF:
    while (samples--)
      {
        float temp[4];
        rgb_to_inks (&config, in, temp);
        spectral_proof    (&config, temp, out);
        in  += 4;
        out += 4;
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
    int i;
    float r = 0, g = 0, b = 0;
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
  } else if (g_str_has_prefix (spectrum, "black"))
  { Spectrum color = TRANSMITTANCE_BLACK; s = color;
  } else if (g_str_has_prefix (spectrum, "gray"))
  { Spectrum color = TRANSMITTANCE_GRAY; s = color;
  } else
  {
    int band = 0;
    band = 0;
    do {
    s.bands[band++] = g_strtod (spectrum, &spectrum);
    } while (spectrum && band < SPECTRUM_BANDS);
  }

  return s;
}

static void parse_config_line (GeglOperation *operation,
                               const char *line)
{
  Spectrum s;
  int i;
  if (!line)
    return;
  if (!strchr (line, '='))
    return;
  while (*line == ' ') line++;
  s = parse_spectrum (strchr (line, '=') +1);
  if (g_str_has_prefix (line, "illuminant"))
    config.illuminant = s;
  else if (g_str_has_prefix (line, "substrate"))
    config.substrate = s;
  else if (g_str_has_prefix (line, "inklimit"))
    {
      config.ink_limit = strchr(line, '=') ? g_strtod (strchr (line, '=')+1, NULL) : 3.0;
      if (config.ink_limit < 0.2)
        config.ink_limit = 0.2;
    }
  else
  for (i = 0; i < 4; i++)
  {
    gchar prefix[] = "ink1";
    prefix[3] = (i+1) + '0';
    if (g_str_has_prefix (line, prefix))
      {
        config.ink_def[i].transmittance = s;
        if (strstr(line, "o=")) config.ink_def[i].opacity = g_strtod (strstr(line, "o=") + 2, NULL);
        if (strstr(line, "s=")) config.ink_def[i].scale = g_strtod (strstr(line, "s=") + 2, NULL);
        config.inks = MAX(config.inks, i+1);
      }
  }
}

static void parse_config (GeglOperation *operation)
{
  GeglChantO *o = GEGL_CHANT_PROPERTIES (operation);
  const char *p = o->config;
  GString *str;
  
  int i;
  if (!p)
    return;

  memset (&config, 0, sizeof (config));
  for (i = 0; i < 4; i++) config.ink_def[i].scale = 1.0;
  config.ink_limit = 3.0;

  str = g_string_new ("");
  while (*p)
  {
    switch (*p)
    {
      case '\n':
        parse_config_line (operation, str->str);
        g_string_assign (str, "");
        break;
      default:
        g_string_append_c (str, *p);
        break;
    }
    p++;
  }

  g_string_free (str, TRUE);
}

static void
gegl_chant_class_init (GeglChantClass *klass)
{
  GeglOperationClass            *operation_class;
  GeglOperationPointFilterClass *point_filter_class;

  operation_class = GEGL_OPERATION_CLASS (klass);
  point_filter_class = GEGL_OPERATION_POINT_FILTER_CLASS (klass);

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
