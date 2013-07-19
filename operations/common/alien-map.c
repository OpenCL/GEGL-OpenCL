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
 * Original authors:
 *
 *      Daniel Cotting <cotting@mygale.org>
 *      Martin Weber   <martweb@gmx.net>
 */

#include "config.h"
#include <glib/gi18n-lib.h>
#include <math.h>

#ifdef GEGL_CHANT_PROPERTIES

gegl_chant_register_enum (gegl_alien_map_color_model)
  enum_value (GEGL_ALIEN_MAP_COLOR_MODEL_RGB, "RGB")
  enum_value (GEGL_ALIEN_MAP_COLOR_MODEL_HSL, "HSL")
gegl_chant_register_enum_end (GeglAlienMapColorModel)

gegl_chant_enum    (color_model, _("Color model"),
                    GeglAlienMapColorModel,
                    gegl_alien_map_color_model,
                    GEGL_ALIEN_MAP_COLOR_MODEL_RGB,
                    _("What color model used for the transformation"))

gegl_chant_double  (cpn_1_frequency, _("Component 1 frequency"),
                    0, 20, 1,
                    _("Component 1 frequency"))
gegl_chant_double  (cpn_2_frequency, _("Component 2 frequency"),
                    0, 20, 1,
                    _("Component 2 frequency"))
gegl_chant_double  (cpn_3_frequency, _("Component 3 frequency"),
                    0, 20, 1,
                    _("Component 3 frequency"))

gegl_chant_double  (cpn_1_phaseshift, _("Component 1 phase shift"),
                    0, 360, 0,
                    _("Component 1 phase shift"))
gegl_chant_double  (cpn_2_phaseshift, _("Component 2 phase shift"),
                    0, 360, 0,
                    _("Component 2 phase shift"))
gegl_chant_double  (cpn_3_phaseshift, _("Component 3 phase shift"),
                    0, 360, 0,
                    _("Component 3 phase shift"))

gegl_chant_boolean (cpn_1_keep, _("Keep component 1"),
                    FALSE,
                    _("Keep component 1"))
gegl_chant_boolean (cpn_2_keep, _("Keep component 2"),
                    FALSE,
                    _("Keep component 2"))
gegl_chant_boolean (cpn_3_keep, _("Keep component 3"),
                    FALSE,
                    _("Keep component 3"))

#else

#define GEGL_CHANT_TYPE_POINT_FILTER
#define GEGL_CHANT_C_FILE "alien-map.c"

#include "gegl-chant.h"

static void
prepare (GeglOperation *operation)
{
  GeglChantO *o = GEGL_CHANT_PROPERTIES (operation);

  if (o->color_model == GEGL_ALIEN_MAP_COLOR_MODEL_RGB)
    {
      gegl_operation_set_format (operation, "input",
                                 babl_format ("R'G'B'A float"));
      gegl_operation_set_format (operation, "output",
                                 babl_format ("R'G'B'A float"));
    }
  else
    {
      gegl_operation_set_format (operation, "input",
                                 babl_format ("HSLA float"));
      gegl_operation_set_format (operation, "output",
                                 babl_format ("HSLA float"));
    }
}

static gboolean
process (GeglOperation       *op,
         void                *in_buf,
         void                *out_buf,
         glong                samples,
         const GeglRectangle *roi,
         gint                 level)
{
  GeglChantO *o   = GEGL_CHANT_PROPERTIES (op);
  gfloat     *in  = in_buf;
  gfloat     *out = out_buf;
  gfloat      freq[3];
  gfloat      phaseshift[3];
  gboolean    keep[3];

  freq[0] = o->cpn_1_frequency * G_PI;
  freq[1] = o->cpn_2_frequency * G_PI;
  freq[2] = o->cpn_3_frequency * G_PI;

  phaseshift[0] = G_PI * o->cpn_1_phaseshift / 180.0;
  phaseshift[1] = G_PI * o->cpn_2_phaseshift / 180.0;
  phaseshift[2] = G_PI * o->cpn_3_phaseshift / 180.0;

  keep[0] = o->cpn_1_keep;
  keep[1] = o->cpn_2_keep;
  keep[2] = o->cpn_3_keep;

  while (samples--)
    {
      gint i;
      for (i = 0; i < 3; i++)
        {
          out[i] = keep[i] ?
            in[i] :
            0.5 * (1.0 + sin ((2 * in[i] - 1.0) * freq[i] + phaseshift[i]));
        }

      out[3] = in[3];

      in  += 4;
      out += 4;
    }

  return TRUE;
}

static void
gegl_chant_class_init (GeglChantClass *klass)
{
  GeglOperationClass            *operation_class;
  GeglOperationPointFilterClass *point_filter_class;

  operation_class    = GEGL_OPERATION_CLASS (klass);
  point_filter_class = GEGL_OPERATION_POINT_FILTER_CLASS (klass);

  operation_class->prepare    = prepare;
  point_filter_class->process = process;

  gegl_operation_class_set_keys (operation_class,
    "name",        "gegl:alien-map",
    "categories",  "artistic",
    "description", _("Alters colors using sine transformations"),
    NULL);
}

#endif
