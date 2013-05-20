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
 * Copyright 2006 Øyvind Kolås <pippin@gimp.org>
 * Copyright 2012 Michael Muré <batolettre@gmail.com>
 */

#include "config.h"
#include <glib/gi18n-lib.h>
#include <math.h>

#ifdef GEGL_CHANT_PROPERTIES

gegl_chant_int_ui (size, _("Block Width"),
                   1, 123456, 16, 1, 1024, 1.5,
                   _("Size of each block in pixels"))

gegl_chant_double (ratio, _("Dot size ratio"),
                   0.0, 1.0, 1.0,
                   _("Size ratio of a dot inside each block"))

#else

#define GEGL_CHANT_TYPE_AREA_FILTER
#define GEGL_CHANT_C_FILE "dot.c"

#include "gegl-chant.h"

#define CELL_X(px, cell_width)  ((px) / (cell_width))
#define CELL_Y(py, cell_height)  ((py) / (cell_height))


static void
prepare (GeglOperation *operation)
{
  GeglChantO              *o;
  GeglOperationAreaFilter *op_area;

  op_area = GEGL_OPERATION_AREA_FILTER (operation);
  o       = GEGL_CHANT_PROPERTIES (operation);

  op_area->left   =
  op_area->right  =
  op_area->top    =
  op_area->bottom = o->size;

  gegl_operation_set_format (operation, "input",
                             babl_format ("RGBA float"));
  gegl_operation_set_format (operation, "output",
                             babl_format ("RGBA float"));
}

static void
calc_block_colors (gfloat* block_colors,
                   const gfloat* input,
                   const GeglRectangle* roi,
                   gint size)
{
  gint cx0 = CELL_X(roi->x, size);
  gint cy0 = CELL_Y(roi->y, size);
  gint cx1 = CELL_X(roi->x + roi->width - 1, size);
  gint cy1 = CELL_Y(roi->y + roi->height - 1, size);

  gint cx;
  gint cy;
  gfloat weight = 1.0f / (size * size);
  gint line_width = roi->width + 2*size;
  /* loop over the blocks within the region of interest */
  for (cy=cy0; cy<=cy1; ++cy)
    {
      for (cx=cx0; cx<=cx1; ++cx)
        {
          gint px = (cx * size) - roi->x + size;
          gint py = (cy * size) - roi->y + size;

          /* calculate the average color for this block */
          gint j,i,c;
          gfloat col[4] = {0.0f, 0.0f, 0.0f, 0.0f};
          for (j=py; j<py+size; ++j)
            {
              for (i=px; i<px+size; ++i)
                {
                  for (c=0; c<4; ++c)
                    col[c] += input[(j*line_width + i)*4 + c];
                }
            }
          for (c=0; c<4; ++c)
            block_colors[c] = weight * col[c];
          block_colors += 4;
        }
    }
}

static void
dot (gfloat* buf,
     const GeglRectangle* roi,
     GeglChantO *o)
{
  gint cx0 = CELL_X(roi->x, o->size);
  gint cy0 = CELL_Y(roi->y, o->size);
  gint block_count_x = CELL_X(roi->x + roi->width - 1, o->size) - cx0 + 1;
  gint block_count_y = CELL_Y(roi->y + roi->height - 1, o->size) - cy0 + 1;
  gfloat* block_colors = g_new0 (gfloat, block_count_x * block_count_y * 4);
  gint x;
  gint y;
  gint c;
  gfloat radius2 = (o->size * o->ratio / 2.0);
  radius2 *= radius2;

/* calculate the average color of all the blocks */
  calc_block_colors(block_colors, buf, roi, o->size);

  /* set each pixel to the average color of the block it belongs to */
  for (y=0; y<roi->height; ++y)
    {
      gint cy = CELL_Y(y + roi->y, o->size) - cy0;
      for (x=0; x<roi->width; ++x)
        {
          gint cx = CELL_X(x + roi->x, o->size) - cx0;
          gfloat cellx = x + roi->x - (cx0 + cx) * o->size - o->size / 2.0;
          gfloat celly = y + roi->y - (cy0 + cy) * o->size - o->size / 2.0;
          
          if (cellx * cellx + celly * celly > radius2)
            for (c=0; c<4; ++c)
              *buf++ = 0;
          else
            for (c=0; c<4; ++c)
              *buf++ = block_colors[(cy*block_count_x + cx)*4 + c];
        }
    }

  g_free (block_colors);
}

static gboolean
process (GeglOperation       *operation,
         GeglBuffer          *input,
         GeglBuffer          *output,
         const GeglRectangle *roi,
         gint                 level)
{
  GeglRectangle src_rect;
  GeglChantO *o = GEGL_CHANT_PROPERTIES (operation);
  GeglOperationAreaFilter *op_area;
  gfloat* buf;

  op_area = GEGL_OPERATION_AREA_FILTER (operation);
  src_rect = *roi;
  src_rect.x -= op_area->left;
  src_rect.y -= op_area->top;
  src_rect.width += op_area->left + op_area->right;
  src_rect.height += op_area->top + op_area->bottom;

  buf = g_new0 (gfloat, src_rect.width * src_rect.height * 4);

  gegl_buffer_get (input, &src_rect, 1.0, babl_format ("RGBA float"), buf, GEGL_AUTO_ROWSTRIDE, GEGL_ABYSS_NONE);

  dot(buf, roi, o);
  gegl_buffer_set (output, roi, 0, babl_format ("RGBA float"), buf, GEGL_AUTO_ROWSTRIDE);

  g_free (buf);

  return  TRUE;
}


static void
gegl_chant_class_init (GeglChantClass *klass)
{
  GeglOperationClass       *operation_class;
  GeglOperationFilterClass *filter_class;

  operation_class = GEGL_OPERATION_CLASS (klass);
  filter_class    = GEGL_OPERATION_FILTER_CLASS (klass);

  filter_class->process    = process;
  operation_class->prepare = prepare;

  gegl_operation_class_set_keys (operation_class,
    "name",        "gegl:dot",
    "categories",  "render",
    "description", _("Simplify image into an array of solid-colored dots"),
    NULL);
}

#endif
