/* This file is an image processing operation for GEGL
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Author: Karl-Johan Andersson <t96kja@student.tdb.uu.se>
 *
 * GEGL port: Thomas Manni <thomas.manni@free.fr>
 *
 */

#include "config.h"
#include <glib/gi18n-lib.h>
#include <math.h>

#ifdef GEGL_PROPERTIES

property_double (pos_x, _("X position"), 0.5)
  description (_("X coordinates of the flare center"))
  ui_range (0.0, 1.0)
  ui_meta  ("unit", "relative-coordinate")
  ui_meta  ("axis", "x")

property_double (pos_y, _("Y position"), 0.5)
  description (_("Y coordinates of the flare center"))
  ui_range (0.0, 1.0)
  ui_meta  ("unit", "relative-coordinate")
  ui_meta  ("axis", "y")

#else

#define GEGL_OP_POINT_FILTER
#define GEGL_OP_C_SOURCE lens-flare.c

#include "gegl-op.h"

#define NUMREF 19

typedef struct
{
  gdouble ccol[3];
  gfloat  size;
  gint    xp;
  gint    yp;
  gint    type;
} Reflect;

typedef struct
{
  Reflect ref[NUMREF];
  gdouble color[3];
  gdouble glow[3];
  gdouble inner[3];
  gdouble outer[3];
  gdouble halo[3];
  gfloat color_size;
  gfloat glow_size;
  gfloat inner_size;
  gfloat outer_size;
  gfloat halo_size;
  gint center_x;
  gint center_y;
} LfParamsType;

static inline void
fixpix (gfloat  *pixel,
        gfloat   procent,
        gdouble *colpro)
{
  pixel[0] += (1.0 - pixel[0]) * procent * colpro[0];
  pixel[1] += (1.0 - pixel[1]) * procent * colpro[1];
  pixel[2] += (1.0 - pixel[2]) * procent * colpro[2];
}

static void
init_params (LfParamsType   *params,
             GeglProperties *o)
{
  Reflect *ref1 = params->ref;

  params->color[0] = 0.937255;
  params->color[1] = 0.937255;
  params->color[2] = 0.937255;
  params->glow[0] = 0.960784;
  params->glow[1] = 0.960784;
  params->glow[2] = 0.960784;
  params->inner[0] = 1.0;
  params->inner[1] = 0.149020;
  params->inner[2] = 0.168627;
  params->outer[0] = 0.270588;
  params->outer[1] = 0.231373;
  params->outer[2] = 0.250980;
  params->halo[0] = 0.313726;
  params->halo[1] = 0.058824;
  params->halo[2] = 0.015686;

  ref1[0].type    = 1;
  ref1[0].ccol[0] = 0.0;
  ref1[0].ccol[1] = 0.054902;
  ref1[0].ccol[2] = 0.443137;

  ref1[1].type    = 1;
  ref1[1].ccol[0] = 0.352941;
  ref1[1].ccol[1] = 0.709804;
  ref1[1].ccol[2] = 0.556863;

  ref1[2].type    = 1;
  ref1[2].ccol[0] = 0.219608;
  ref1[2].ccol[1] = 0.549020;
  ref1[2].ccol[2] = 0.415686;

  ref1[3].type    = 2;
  ref1[3].ccol[0] = 0.035294;
  ref1[3].ccol[1] = 0.113725;
  ref1[3].ccol[2] = 0.074510;

  ref1[4].type    = 2;
  ref1[4].ccol[0] = 0.094118;
  ref1[4].ccol[1] = 0.054902;
  ref1[4].ccol[2] = 0.0;

  ref1[5].type    = 2;
  ref1[5].ccol[0] = 0.094118;
  ref1[5].ccol[1] = 0.054902;
  ref1[5].ccol[2] = 0.0;

  ref1[6].type    = 2;
  ref1[6].ccol[0] = 0.164706;
  ref1[6].ccol[1] = 0.074510;
  ref1[6].ccol[2] = 0.0;

  ref1[7].type    = 2;
  ref1[7].ccol[0] = 0.0;
  ref1[7].ccol[1] = 0.035294;
  ref1[7].ccol[2] = 0.066667;

  ref1[8].type    = 2;
  ref1[8].ccol[0] = 0.0;
  ref1[8].ccol[1] = 0.015686;
  ref1[8].ccol[2] = 0.039216;

  ref1[9].type    = 2;
  ref1[9].ccol[0] = 0.019608;
  ref1[9].ccol[1] = 0.019608;
  ref1[9].ccol[2] = 0.054902;

  ref1[10].type    = 2;
  ref1[10].ccol[0] = 0.035294;
  ref1[10].ccol[1] = 0.015686;
  ref1[10].ccol[2] = 0.0;

  ref1[11].type    = 2;
  ref1[11].ccol[0] = 0.035294;
  ref1[11].ccol[1] = 0.015686;
  ref1[11].ccol[2] = 0.0;

  ref1[12].type    = 3;
  ref1[12].ccol[0] = 0.133333;
  ref1[12].ccol[1] = 0.074510;
  ref1[12].ccol[2] = 0.0;

  ref1[13].type    = 3;
  ref1[13].ccol[0] = 0.054902;
  ref1[13].ccol[1] = 0.101961;
  ref1[13].ccol[2] = 0.0;

  ref1[14].type    = 3;
  ref1[14].ccol[0] = 0.039216;
  ref1[14].ccol[1] = 0.098039;
  ref1[14].ccol[2] = 0.050980;

  ref1[15].type    = 4;
  ref1[15].ccol[0] = 0.035294;
  ref1[15].ccol[1] = 0.0;
  ref1[15].ccol[2] = 0.066667;

  ref1[16].type    = 4;
  ref1[16].ccol[0] = 0.035294;
  ref1[16].ccol[1] = 0.062745;
  ref1[16].ccol[2] = 0.019608;

  ref1[17].type   = 4;
  ref1[17].ccol[0] = 0.066667;
  ref1[17].ccol[1] = 0.015686;
  ref1[17].ccol[2] = 0.0;

  ref1[18].type   = 4;
  ref1[18].ccol[0] = 0.066667;
  ref1[18].ccol[1] = 0.015686;
  ref1[18].ccol[2] = 0.0;
}

static void
update_params (gdouble sx,
               gdouble sy,
               gint    width,
               gint    height,
               LfParamsType *params)
{
  gint xh, yh, dx, dy, matt;
  Reflect *ref1 = params->ref;

  params->center_x = gegl_coordinate_relative_to_pixel(sx,
                                                       width);
  params->center_y = gegl_coordinate_relative_to_pixel(sy,
                                                       height);

  matt = width;

  params->color_size = (gfloat) matt * 0.0375;
  params->glow_size  = (gfloat) matt * 0.078125;
  params->inner_size = (gfloat) matt * 0.1796875;
  params->outer_size = (gfloat) matt * 0.3359375;
  params->halo_size  = (gfloat) matt * 0.084375;

  xh = width / 2;
  yh = height / 2;
  dx = xh - params->center_x;
  dy = yh - params->center_y;

  ref1[0].size    = (gfloat) matt * 0.027;
  ref1[0].xp      = 0.6699 * dx + xh;
  ref1[0].yp      = 0.6699 * dy + yh;

  ref1[1].size    = (gfloat) matt * 0.01;
  ref1[1].xp      = 0.2692 * dx + xh;
  ref1[1].yp      = 0.2692 * dy + yh;

  ref1[2].size    = (gfloat) matt * 0.005;
  ref1[2].xp      = -0.0112 * dx + xh;
  ref1[2].yp      = -0.0112 * dy + yh;

  ref1[3].size    = (gfloat) matt * 0.031;
  ref1[3].xp      = 0.6490 * dx + xh;
  ref1[3].yp      = 0.6490 * dy + yh;

  ref1[4].size    = (gfloat) matt * 0.015;
  ref1[4].xp      = 0.4696 * dx + xh;
  ref1[4].yp      = 0.4696 * dy + yh;

  ref1[5].size    = (gfloat) matt * 0.037;
  ref1[5].xp      = 0.4087 * dx + xh;
  ref1[5].yp      = 0.4087 * dy + yh;

  ref1[6].size    = (gfloat) matt * 0.022;
  ref1[6].xp      = -0.2003 * dx + xh;
  ref1[6].yp      = -0.2003 * dy + yh;

  ref1[7].size    = (gfloat) matt * 0.025;
  ref1[7].xp      = -0.4103 * dx + xh;
  ref1[7].yp      = -0.4103 * dy + yh;

  ref1[8].size    = (gfloat) matt * 0.058;
  ref1[8].xp      = -0.4503 * dx + xh;
  ref1[8].yp      = -0.4503 * dy + yh;

  ref1[9].size    = (gfloat) matt * 0.017;
  ref1[9].xp      = -0.5112 * dx + xh;
  ref1[9].yp      = -0.5112 * dy + yh;

  ref1[10].size    = (gfloat) matt * 0.2;
  ref1[10].xp      = -1.496 * dx + xh;
  ref1[10].yp      = -1.496 * dy + yh;

  ref1[11].size    = (gfloat) matt * 0.5;
  ref1[11].xp      = -1.496 * dx + xh;
  ref1[11].yp      = -1.496 * dy + yh;

  ref1[12].size    = (gfloat) matt * 0.075;
  ref1[12].xp      = 0.4487 * dx + xh;
  ref1[12].yp      = 0.4487 * dy + yh;

  ref1[13].size    = (gfloat) matt * 0.1;
  ref1[13].xp      = dx + xh;
  ref1[13].yp      = dy + yh;

  ref1[14].size    = (gfloat) matt * 0.039;
  ref1[14].xp      = -1.301 * dx + xh;
  ref1[14].yp      = -1.301 * dy + yh;

  ref1[15].size    = (gfloat) matt * 0.19;
  ref1[15].xp      = 1.309 * dx + xh;
  ref1[15].yp      = 1.309 * dy + yh;

  ref1[16].size    = (gfloat) matt * 0.195;
  ref1[16].xp      = 1.309 * dx + xh;
  ref1[16].yp      = 1.309 * dy + yh;

  ref1[17].size   = (gfloat) matt * 0.20;
  ref1[17].xp     = 1.309 * dx + xh;
  ref1[17].yp     = 1.309 * dy + yh;

  ref1[18].size   = (gfloat) matt * 0.038;
  ref1[18].xp     = -1.301 * dx + xh;
  ref1[18].yp     = -1.301 * dy + yh;
}

static void
prepare (GeglOperation *operation)
{
  GeglProperties *o      = GEGL_PROPERTIES (operation);
  const Babl     *format = babl_format ("R'G'B'A float");
  LfParamsType   *params = NULL;

  if (o->user_data == NULL)
    o->user_data = g_slice_new0 (LfParamsType);

  params = (LfParamsType *) o->user_data;

  init_params (params, o);

  gegl_operation_set_format (operation, "input", format);
  gegl_operation_set_format (operation, "output", format);
}

static void
finalize (GObject *object)
{
  GeglOperation *op = (void*) object;
  GeglProperties *o = GEGL_PROPERTIES (op);

  if (o->user_data)
    {
      g_slice_free (LfParamsType, o->user_data);
      o->user_data = NULL;
    }

  G_OBJECT_CLASS (gegl_op_parent_class)->finalize (object);
}

static gboolean
process (GeglOperation       *operation,
         void                *in_buf,
         void                *out_buf,
         glong                n_pixels,
         const GeglRectangle *roi,
         gint                 level)
{
  GeglProperties *o      = GEGL_PROPERTIES (operation);
  LfParamsType   *params = (LfParamsType *) o->user_data;
  GeglRectangle  *whole_region;

  gint    x, y, b, idx, i;
  gfloat  hyp;
  gfloat  procent;
  gfloat *pixel;
  gfloat *input = in_buf;
  gfloat *output = out_buf;

  whole_region = gegl_operation_source_get_bounding_box (operation, "input");

  update_params (o->pos_x,
                 o->pos_y,
                 whole_region->width,
                 whole_region->height,
                 params);

  pixel = g_new (gfloat, 3);

  for (y = 0; y < roi->height; y++)
    {
      for (x = 0; x < roi->width; x++)
        {
          idx = (x + y * roi->width) * 4;

          for (b = 0; b < 3; b++)
            pixel[b] = input[idx + b];

          hyp = hypot (x + roi->x - params->center_x,
                       y + roi->y - params->center_y);

          /* center color */
          procent = params->color_size - hyp;
          procent /= params->color_size;
          if (procent > 0.0)
            {
              procent *= procent;
              fixpix (pixel, procent, params->color);
            }

          /* glow color */
          procent = params->glow_size - hyp;
          procent /= params->glow_size;
          if (procent > 0.0)
            {
              procent *= procent;
              fixpix (pixel, procent, params->glow);
            }

          /* inner color */
          procent = params->inner_size - hyp;
          procent /= params->inner_size;
          if (procent > 0.0)
            {
              procent *= procent;
              fixpix (pixel, procent, params->inner);
            }

          /* outer color */
          procent = params->outer_size - hyp;
          procent /= params->outer_size;
          if (procent > 0.0)
            fixpix (pixel, procent, params->outer);

          /* halo color */
          procent = hyp - params->halo_size;
          procent /= (params->halo_size * 0.07);
          procent  = fabs (procent);
          if (procent < 1.0)
            fixpix (pixel, 1.0 - procent, params->halo);

          /* generate reflections */

          for (i = 0; i < NUMREF; i++)
            {
              Reflect r = params->ref[i];
              gdouble hyp = hypot (x + roi->x - r.xp,
                                   y + roi->y - r.yp);
              switch (r.type)
                {
                  case 1:
                    procent = r.size - hyp;
                    procent /= r.size;
                    if (procent > 0.0)
                      {
                        procent *= procent;
                        fixpix (pixel, procent, (gdouble *) &r.ccol);
                      }
                    break;
                  case 2:
                    procent  = r.size - hyp;
                    procent /= (r.size * 0.15);

                    if (procent > 0.0)
                      {
                        if (procent > 1.0)
                          procent = 1.0;

                        fixpix (pixel, procent, (gdouble *) &r.ccol);
                      }
                    break;
                  case 3:
                    procent  = r.size - hyp;
                    procent /= (r.size * 0.12);

                    if (procent > 0.0)
                      {
                        if (procent > 1.0)
                          procent = 1.0 - (procent * 0.12);

                        fixpix (pixel, procent, (gdouble *) &r.ccol);
                      }
                    break;
                  case 4:
                    procent  = hyp - r.size;
                    procent /= (r.size * 0.04);
                    procent  = fabs (procent);

                   if (procent < 1.0)
                     fixpix (pixel, 1.0 - procent, (gdouble *) &r.ccol);
                   break;
                }
            }

          /* write result to output */

          for (b = 0; b < 3; b++)
            output[idx + b] = pixel[b];

          output[idx + 3] = input[idx + 3];
        }
    }

  g_free (pixel);

  return TRUE;
}

static void
gegl_op_class_init (GeglOpClass *klass)
{
  GObjectClass                  *object_class;
  GeglOperationClass            *operation_class;
  GeglOperationPointFilterClass *filter_class;

  object_class    = G_OBJECT_CLASS (klass);
  operation_class = GEGL_OPERATION_CLASS (klass);
  filter_class    = GEGL_OPERATION_POINT_FILTER_CLASS (klass);

  object_class->finalize = finalize;

  operation_class->prepare = prepare;
  operation_class->opencl_support = FALSE;

  filter_class->process    = process;

  gegl_operation_class_set_keys (operation_class,
    "name",        "gegl:lens-flare",
    "title",       _("Lens Flare"),
    "categories",  "light",
    "license",     "GPL3+",
    "description", _("Adds a lens flare effect."),
    NULL);
}

#endif
