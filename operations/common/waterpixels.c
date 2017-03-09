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
 * Copyright 2016 Thomas Manni <thomas.manni@free.fr>
 */

/* partial implementation of the algorithm described on the paper 
 * "WATERPIXELS: SUPERPIXELS BASED ON THE WATERSHED TRANSFORMATION"
 * written by V. Machairas, E. Decencière and T. Walter.
 */

/* FIXME: spatial regularization influence
 * TODO: option to add superpixels boundaries to the output
 */

#include "config.h"
#include <glib/gi18n-lib.h>

#ifdef GEGL_PROPERTIES

enum_start (gegl_waterpixels_fill)
   enum_value (GEGL_WATERPIXELS_FILL_AVERAGE, "average", N_("Average"))
   enum_value (GEGL_WATERPIXELS_FILL_RANDOM,  "random",  N_("Random"))
enum_end (GeglWaterpixelsFill)

property_int (size, _("Superpixels size"), 32)
  value_range (8, G_MAXINT)
  ui_range (8, 256)

property_double (smoothness, _("Gradient smoothness"), 1.0)
  value_range (0.0, 1000.0)
  ui_range (0.0, 10.0)
  ui_gamma (1.5)

property_int (regularization, _("Spatial regularization"), 0)
  value_range (0, 50)
  description (_("trade-off between superpixel regularity and "
                 "adherence to object boundaries"))

property_enum (fill, _("Superpixels color"),
               GeglWaterpixelsFill, gegl_waterpixels_fill,
               GEGL_WATERPIXELS_FILL_AVERAGE)
  description (_("How to fill superpixels"))

#else

#define GEGL_OP_FILTER
#define GEGL_OP_NAME     waterpixels
#define GEGL_OP_C_SOURCE waterpixels.c

#include "gegl-op.h"
#include <math.h>

#define POW2(x) ((x)*(x))

typedef struct _Cell
{
  gint          center_x;
  gint          center_y;
  GeglRectangle area;
  gfloat        color[3];
  glong         n_pixels;
} Cell;

typedef struct _CellsGrid
{
  Cell  *cells;
  gint   n_cells;
  gint   cell_size;
  gint   cells_per_row;
  gint   cells_per_column;
} CellsGrid;

static void
initiliaze_cellsgrid (CellsGrid           *grid,
                      const GeglRectangle *input_extent,
                      gint                 cell_size)
{
  gint cells_per_row;
  gint cells_per_column;
  gint x, y;
  gint half_size = cell_size / 2;
  gint two_third_size = cell_size * 2 / 3 ;

  cells_per_row = input_extent->width / cell_size;
  if (input_extent->width % cell_size)
    cells_per_row++;

  cells_per_column = input_extent->height / cell_size;
  if (input_extent->height % cell_size)
    cells_per_column++;

  grid->n_cells          = cells_per_row * cells_per_column;
  grid->cells            = g_new0 (Cell, grid->n_cells);
  grid->cell_size        = cell_size;
  grid->cells_per_row    = cells_per_row;
  grid->cells_per_column = cells_per_column;

  for (y = 0; y < grid->cells_per_column; y++)
    for (x = 0; x < grid->cells_per_row; x++)
      {
        gint  i    = x + y * grid->cells_per_row;
        Cell *cell = grid->cells + i;

        cell->center_x = x * cell_size + half_size;
        cell->center_y = y * cell_size + half_size;

        cell->area.x   = x * cell_size + cell_size / 6;
        cell->area.y   = y * cell_size + cell_size / 6;
        cell->area.width  = two_third_size;
        cell->area.height = two_third_size;

        gegl_rectangle_intersect (&grid->cells[i].area,
                                  &grid->cells[i].area,
                                  input_extent);
      }
}

static GeglBuffer *
generate_gradient (GeglBuffer *input,
                   gdouble     smoothness)
{
  GeglBuffer *gradient;
  GeglNode   *gegl;
  GeglNode   *source;
  GeglNode   *write;
  GeglNode   *blur;
  GeglNode   *gradient_magnitude;

  gradient = gegl_buffer_new (gegl_buffer_get_extent (input),
                              babl_format ("Y float"));

  gegl = gegl_node_new ();

  source = gegl_node_new_child (gegl,
                                "operation", "gegl:buffer-source",
                                "buffer", input,
                                 NULL);

  blur = gegl_node_new_child (gegl,
                              "operation", "gegl:gaussian-blur",
                              "std-dev-x", smoothness,
                              "std-dev-y", smoothness,
                              NULL);

  gradient_magnitude = gegl_node_new_child (gegl,
                                  "operation", "gegl:image-gradient",
                                  NULL);

  write = gegl_node_new_child (gegl,
                              "operation", "gegl:write-buffer",
                              "buffer", gradient,
                              NULL);

  gegl_node_link_many (source, blur, gradient_magnitude, write, NULL);
  gegl_node_process (write);
  g_object_unref (gegl);

  return gradient;
}

static void
regularize_gradient  (GeglBuffer *gradient,
                      gint32      regularization,
                      CellsGrid  *grid)
{
  GeglBufferIterator *iter;
  gint x, y;

  iter = gegl_buffer_iterator_new (gradient, NULL, 0, babl_format ("Y float"),
                                   GEGL_ACCESS_READWRITE, GEGL_ABYSS_NONE);

  while (gegl_buffer_iterator_next (iter))
    {
      gfloat  *pixel = iter->data[0];

      for (y = iter->roi->y; y < iter->roi->y + iter->roi->height; y++)
        for (x = iter->roi->x; x < iter->roi->x + iter->roi->width; x++)
          {
            gint X = x / grid->cell_size;
            gint Y = y / grid->cell_size;

            Cell *cell = grid->cells + X + Y * grid->cells_per_row;

            //gdouble distance = (POW2(x - cell->center_x) + POW2(y - cell->center_y)) / 255.0;
            gdouble distance = sqrt (POW2(x - cell->center_x)
                                     + POW2(y - cell->center_y))
                                / (gdouble) grid->cell_size;

           *pixel = *pixel + regularization * 2.0 * distance / (gdouble) grid->cell_size;

            pixel++;
          }
    }
}

static GeglBuffer *
generate_labels (GeglBuffer *gradient,
                 CellsGrid  *grid)
{
  GeglBuffer  *labels;
  gfloat      *buff;
  guint32      i;
  guint32      label[2];

  labels = gegl_buffer_new (gegl_buffer_get_extent (gradient),
                            babl_format ("YA u32"));

  for (i = 0; i < grid->n_cells; i++)
    {
      Cell *cell   = grid->cells + i;
      GeglRectangle min_pixel = {0, 0, 1, 1};
      gfloat min_value = G_MAXFLOAT;
      gfloat  *pixel;
      gint x = cell->area.x;
      gint y = cell->area.y;
      gint n_pixels = cell->area.width * cell->area.height;

      buff = g_new (gfloat, n_pixels);

      gegl_buffer_get (gradient, &cell->area, 1.0, babl_format ("Y float"),
                       buff, GEGL_AUTO_ROWSTRIDE, GEGL_ABYSS_NONE);

      pixel = buff;

      while (n_pixels--)
        {
          if (*pixel < min_value)
            {
              min_value = *pixel;
              min_pixel.x = x;
              min_pixel.y = y;
            }

          pixel++;
          x++;

          if (x >= cell->area.x + cell->area.width)
            {
              x = cell->area.x;
              y++;
            }
        }

      label[0] = i;
      label[1] = 1;
      gegl_buffer_set (labels, &min_pixel, 0, babl_format ("YA u32"),
                       label, GEGL_AUTO_ROWSTRIDE);

      g_free (buff);
    }

  return labels;
}


static GeglBuffer *
propagate_labels (GeglBuffer *labels,
                  GeglBuffer *gradient)
{
  GeglNode   *gegl;
  GeglNode   *source_labels;
  GeglNode   *source_gradient;
  GeglNode   *write;
  GeglNode   *watershed;

  GeglBuffer *result = gegl_buffer_new (gegl_buffer_get_extent (labels),
                       babl_format ("YA u32"));

  gegl = gegl_node_new ();

  source_labels = gegl_node_new_child (gegl,
                                "operation", "gegl:buffer-source",
                                "buffer", labels,
                                 NULL);

  source_gradient = gegl_node_new_child (gegl,
                                "operation", "gegl:buffer-source",
                                "buffer", gradient,
                                 NULL);

  watershed = gegl_node_new_child (gegl,
                              "operation", "gegl:watershed-transform",
                              NULL);

  write = gegl_node_new_child (gegl,
                              "operation", "gegl:write-buffer",
                              "buffer", result,
                              NULL);

  gegl_node_link_many (source_labels, watershed, write, NULL);
  gegl_node_connect_from (watershed, "aux", source_gradient, "output");
  gegl_node_process (write);
  g_object_unref (gegl);
  return result;
}

static void
get_random_colors (CellsGrid  *grid)
{
  GeglRandom *gr;
  gint        i;

  gr = gegl_random_new ();

  for (i = 0; i < grid->n_cells; i++)
    {
      Cell *cell = grid->cells + i;

      cell->color[0] = gegl_random_float_range (gr,
                                          cell->center_x,
                                          cell->center_y,
                                          i, 0, 0.0, 1.0);
      cell->color[1] = gegl_random_float_range (gr,
                                          cell->center_x+1,
                                          cell->center_y+1,
                                          i+1, 0, 0.0, 1.0);
      cell->color[2] = gegl_random_float_range (gr,
                                          cell->center_x+2,
                                          cell->center_y+2,
                                          i+2, 0, 0.0, 1.0);
    }

  gegl_random_free (gr);
}

static void
get_average_colors (GeglBuffer *input,
                    GeglBuffer *labels,
                    CellsGrid  *grid)
{
  GeglBufferIterator *iter;
  gint                i;

  iter = gegl_buffer_iterator_new (labels, gegl_buffer_get_extent (labels),
                                   0, babl_format ("YA u32"),
                                   GEGL_ACCESS_READ, GEGL_ABYSS_NONE);

  gegl_buffer_iterator_add (iter, input, gegl_buffer_get_extent (labels), 0,
                            babl_format ("R'G'B' float"),
                            GEGL_ACCESS_READ, GEGL_ABYSS_NONE);

  while (gegl_buffer_iterator_next (iter))
    {
      guint32  *label    = iter->data[0];
      gfloat   *pixel    = iter->data[1];
      glong     n_pixels = iter->length;

      while (n_pixels--)
        {
          Cell *cell = grid->cells + label[0];

          cell->color[0] += pixel[0];
          cell->color[1] += pixel[1];
          cell->color[2] += pixel[2];

          cell->n_pixels++;

          pixel += 3;
          label += 2;
        }
    }

  for (i = 0; i < grid->n_cells; i++)
    {
      Cell *cell = grid->cells + i;
      cell->color[0] /= cell->n_pixels;
      cell->color[1] /= cell->n_pixels;
      cell->color[2] /= cell->n_pixels;
    }
}

static void
fill_output (GeglBuffer *output,
             GeglBuffer *labels,
             CellsGrid  *grid)
{
  GeglBufferIterator *iter;

  iter = gegl_buffer_iterator_new (labels, NULL, 0, babl_format ("YA u32"),
                                   GEGL_ACCESS_READ, GEGL_ABYSS_NONE);

  gegl_buffer_iterator_add (iter, output, NULL, 0,
                            babl_format ("R'G'B' float"),
                            GEGL_ACCESS_WRITE, GEGL_ABYSS_NONE);

  while (gegl_buffer_iterator_next (iter))
    {
      guint32  *label    = iter->data[0];
      gfloat   *pixel    = iter->data[1];
      glong     n_pixels = iter->length;

      while (n_pixels--)
        {
          Cell *cell = grid->cells + label[0];

          pixel[0] = cell->color[0];
          pixel[1] = cell->color[1];
          pixel[2] = cell->color[2];

          pixel += 3;
          label += 2;
        }
    }
}

static void
prepare (GeglOperation *operation)
{
  const Babl *format = babl_format ("R'G'B' float");

  gegl_operation_set_format (operation, "input",  format);
  gegl_operation_set_format (operation, "output", format);
}

static GeglRectangle
get_required_for_output (GeglOperation       *operation,
                         const gchar         *input_pad,
                         const GeglRectangle *roi)
{
  GeglRectangle result = *gegl_operation_source_get_bounding_box (operation, "input");

  /* Don't request an infinite plane */
  if (gegl_rectangle_is_infinite_plane (&result))
    return *roi;

  return result;
}

static GeglRectangle
get_cached_region (GeglOperation       *operation,
                   const GeglRectangle *roi)
{
  GeglRectangle result = *gegl_operation_source_get_bounding_box (operation, "input");

  if (gegl_rectangle_is_infinite_plane (&result))
    return *roi;

  return result;
}

static gboolean
process (GeglOperation       *operation,
         GeglBuffer          *input,
         GeglBuffer          *output,
         const GeglRectangle *roi,
         gint                 level)
{
  GeglProperties  *o = GEGL_PROPERTIES (operation);

  GeglBuffer *gradient;
  GeglBuffer *initial_labels;
  GeglBuffer *propagated_labels;
  CellsGrid   grid;

  initiliaze_cellsgrid (&grid, gegl_buffer_get_extent (input), o->size);

  gradient       = generate_gradient (input, o->smoothness);
  initial_labels = generate_labels (gradient, &grid);

  if (o->regularization)
    regularize_gradient (gradient, o->regularization, &grid);

  propagated_labels = propagate_labels (initial_labels, gradient);

  if (o->fill == GEGL_WATERPIXELS_FILL_RANDOM)
    get_random_colors (&grid);
  else
    get_average_colors (input, propagated_labels, &grid);

  fill_output (output, propagated_labels, &grid);

  g_object_unref (gradient);
  g_object_unref (initial_labels);
  g_object_unref (propagated_labels);
  g_free (grid.cells);

  return TRUE;
}

static void
gegl_op_class_init (GeglOpClass *klass)
{
  GeglOperationClass       *operation_class;
  GeglOperationFilterClass *filter_class;

  operation_class = GEGL_OPERATION_CLASS (klass);
  filter_class    = GEGL_OPERATION_FILTER_CLASS (klass);

  filter_class->process                    = process;
  operation_class->prepare                 = prepare;
  operation_class->get_required_for_output = get_required_for_output;
  operation_class->get_cached_region       = get_cached_region;
  operation_class->opencl_support          = FALSE;

  gegl_operation_class_set_keys (operation_class,
    "name",        "gegl:waterpixels",
    "title",       _("Waterpixels"),
    "categories",  "segmentation",
    "reference-hash", "9aac02fb4816ea0b1142d86a55495072",
    "description", _("Superpixels based on the watershed transformation"),
    NULL);
}

#endif
