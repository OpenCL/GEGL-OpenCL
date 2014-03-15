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
"illuminant = 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1\n" \
"substrate  = white\n" \
"ink1 = cyan 1.8\n"\
"ink2 = magenta 1.8\n"\
"ink3 = yellow 2.2\n"\
"ink4 = black opaque\n"

gegl_chant_multiline (config, _("ink configuration"), DEFAULT_INKS,
         _("Textual desciption of inks passed in"))

#else

#define GEGL_CHANT_TYPE_POINT_FILTER
#define GEGL_CHANT_C_FILE       "ink-simulator.c"

#include "gegl-chant.h"

#define SPECTRUM_BANDS 20

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
static Spectrum ILLUMINANT            = ILLUMINANT_E;
static Spectrum SUBSTRATE_TRANSMITTANCE = TRANSMITTANCE_WHITE;

static int inks = 3;
static Spectrum INK_TRANSMITTANCE_INK1 = TRANSMITTANCE_GREEN;
static Spectrum INK_TRANSMITTANCE_INK2 = TRANSMITTANCE_BLACK;
static Spectrum INK_TRANSMITTANCE_INK3 = TRANSMITTANCE_YELLOW;
static Spectrum INK_TRANSMITTANCE_INK4 = TRANSMITTANCE_WHITE;
static Spectrum INK_TRANSMITTANCE_INK5 = TRANSMITTANCE_WHITE;
static int ink1_opaque = 0;
static int ink2_opaque = 0;
static int ink3_opaque = 0;
static int ink4_opaque = 0;
static int ink5_opaque = 0;


static Spectrum spectrum_remove_light (Spectrum s, Spectrum ink, float factor)
{
  int i;
  for (i = 0; i < SPECTRUM_BANDS; i++)
  {
    s.bands[i] = s.bands[i] * (1.0 - factor) +
                 s.bands[i] * ink.bands[i] * factor;
  }
  return s;
}

static Spectrum spectrum_add_color (Spectrum s, Spectrum ink, float factor)
{
  int i;
  for (i = 0; i < SPECTRUM_BANDS; i++)
  {
    s.bands[i] = s.bands[i] * (1.0 - factor) + ink.bands[i] * factor;
  }
  return s;
}

static float spectrum_observed_l (Spectrum s)
{
  float result = 0.0;
  int i;
  for (i = 0; i < SPECTRUM_BANDS; i++)
    result += s.bands[i] * STANDARD_OBSERVER_L.bands[i];
  return (result / SPECTRUM_BANDS) * 3;
}

static float spectrum_observed_m (Spectrum s)
{
  float result = 0.0;
  int i;
  for (i = 0; i < SPECTRUM_BANDS; i++)
    result += s.bands[i] * STANDARD_OBSERVER_M.bands[i];
  return (result / SPECTRUM_BANDS) * 3;
}

static float spectrum_observed_s (Spectrum s)
{
  float result = 0.0;
  int i;
  for (i = 0; i < SPECTRUM_BANDS; i++)
    result += s.bands[i] * STANDARD_OBSERVER_S.bands[i];
  return (result / SPECTRUM_BANDS) * 3;
}

static void parse_config (GeglOperation *operation);

static void
prepare (GeglOperation *operation)
{

  /* this RGBA float is interpreted as CMYK ! */
  gegl_operation_set_format (operation, "input", babl_format ("RGBA float"));

  gegl_operation_set_format (operation, "output", babl_format ("RGB float"));


  parse_config (operation);
}

static float ink_scale[4] = {1,1,1,1};

static gboolean
process (GeglOperation       *op,
         void                *in_buf,
         void                *out_buf,
         glong                samples,
         const GeglRectangle *roi,
         gint                 level)
{
  gfloat *in  = in_buf;
  gfloat *out = out_buf;

  while (samples--)
    {
      Spectrum spec = ILLUMINANT;
      float l, m, s;

      spec = spectrum_remove_light (spec, SUBSTRATE_TRANSMITTANCE, 1.0);
      if (inks >=1)
      {
        if (ink1_opaque)
          spec = spectrum_add_color (spec, INK_TRANSMITTANCE_INK1, in[0] * ink_scale[0]);
        else
          spec = spectrum_remove_light (spec, INK_TRANSMITTANCE_INK1, in[0] * ink_scale[0]);
      }
      if (inks >=2)
      {
        if (ink2_opaque)
          spec = spectrum_add_color (spec, INK_TRANSMITTANCE_INK2, in[1] * ink_scale[1]);
        else
          spec = spectrum_remove_light (spec, INK_TRANSMITTANCE_INK2, in[1] * ink_scale[1]);
      }
      if (inks >=3)
      {
        if (ink3_opaque)
          spec = spectrum_add_color (spec, INK_TRANSMITTANCE_INK3, in[2] * ink_scale[2]);
        else
          spec = spectrum_remove_light (spec, INK_TRANSMITTANCE_INK3, in[2] * ink_scale[2]);
      }
      if (inks >=4)
      {
        if (ink4_opaque)
          spec = spectrum_add_color (spec, INK_TRANSMITTANCE_INK4, in[3] * ink_scale[3]);
        else
          spec = spectrum_remove_light (spec, INK_TRANSMITTANCE_INK4, in[3] * ink_scale[3]);
      }

      l = spectrum_observed_l (spec) /6;
      m = spectrum_observed_m (spec) /6;
      s = spectrum_observed_s (spec) /6;

      /* this LMS to linear RGB -- likely no right..  */
      
      out[0] = ((30.83) * l + (-29.83) * m + (1.61) * s);
      out[1] = ((-6.481) * l + (17.7155) * m + (-2.532) * s);
      out[2] = ((-0.375) * l + (-1.19906) * m + (14.27) * s);


      in  += 4;
      out += 3;
    }
  return TRUE;
}


static Spectrum parse_spectrum (gchar *spectrum, float *scale)
{
  Spectrum s = TRANSMITTANCE_WHITE;
  if (!spectrum)
    return s;
  while (*spectrum == ' ') spectrum ++;

  if (g_str_has_prefix (spectrum, "red"))
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

  *scale = 1.0;
  if (strrchr(spectrum, ' '))
  {
    char *p = strrchr(spectrum, ' ');
    if (atof (p)) *scale = atof (p);
  }

  return s;
}

static void parse_config_line (GeglOperation *operation,
                               const char *line)
{
  Spectrum s;
  float scale = 1.0;
  if (!line)
    return;

  if (!strchr (line, '='))
    return;

  while (*line == ' ') line++;
  
  s = parse_spectrum (strchr (line, '=') + 1, &scale);

  inks = 0;

  if (g_str_has_prefix (line, "illuminant"))
    ILLUMINANT = s;
  else if (g_str_has_prefix (line, "substrate"))
    SUBSTRATE_TRANSMITTANCE = s;
  else if (g_str_has_prefix (line, "ink1"))
    {
      INK_TRANSMITTANCE_INK1 = s;
      if (strstr(line, "opaque"))
        ink1_opaque = 1;
      inks = MAX(inks, 1);
      ink_scale[0] = scale;
    }
  else if (g_str_has_prefix (line, "ink2"))
    {
      INK_TRANSMITTANCE_INK2 = s;
      inks = MAX(inks, 2);
      if (strstr(line, "opaque"))
        ink2_opaque = 2;
      ink_scale[1] = scale;
    }
  else if (g_str_has_prefix (line, "ink3"))
    {
      INK_TRANSMITTANCE_INK3 = s;
      inks = MAX(inks, 3);
      if (strstr(line, "opaque"))
        ink3_opaque = 3;
      ink_scale[2] = scale;
    }
  else if (g_str_has_prefix (line, "ink4"))
    {
      INK_TRANSMITTANCE_INK4 = s;
      inks = MAX(inks, 4);
      if (strstr(line, "opaque"))
        ink4_opaque = 4;
      ink_scale[3] = scale;
    }
}

static void parse_config (GeglOperation *operation)
{
  GeglChantO *o = GEGL_CHANT_PROPERTIES (operation);
  const char *p = o->config;
  GString *str;
  
  if (!p)
    return;

  memset (&INK_TRANSMITTANCE_INK1, 0, sizeof (INK_TRANSMITTANCE_INK1));
  memset (&INK_TRANSMITTANCE_INK2, 0, sizeof (INK_TRANSMITTANCE_INK2));
  memset (&INK_TRANSMITTANCE_INK3, 0, sizeof (INK_TRANSMITTANCE_INK3));
  memset (&INK_TRANSMITTANCE_INK4, 0, sizeof (INK_TRANSMITTANCE_INK4));
  memset (&INK_TRANSMITTANCE_INK5, 0, sizeof (INK_TRANSMITTANCE_INK5));
  ink1_opaque = 0;
  ink2_opaque = 0;
  ink3_opaque = 0;
  ink4_opaque = 0;
  ink5_opaque = 0;

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
