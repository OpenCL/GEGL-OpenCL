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

 /* Propagate labels by wathershed transformation using hierarchical queues */

#include "config.h"
#include <glib/gi18n-lib.h>

#ifdef GEGL_PROPERTIES

#else

#define GEGL_OP_FILTER
#define GEGL_OP_NAME      watershed_transform
#define GEGL_OP_C_SOURCE  watershed-transform.c

#include "gegl-op.h"

typedef struct _PixelCoords
{
  gint x;
  gint y;
} PixelCoords;

typedef struct _HQ
{
  GQueue  *queues[256];
  GQueue  *lowest_non_empty;
  gint     lowest_non_empty_level;
} HQ;

static void
HQ_init (HQ *hq)
{
  gint i;

  for (i = 0; i < 256; i++)
    hq->queues[i] = g_queue_new ();

  hq->lowest_non_empty       = NULL;
  hq->lowest_non_empty_level = 255;
}

static gboolean
HQ_is_empty (HQ *hq)
{
  if (hq->lowest_non_empty == NULL)
    return TRUE;

  return FALSE;
}

static inline void
HQ_push (HQ      *hq,
         guint8   level,
         gpointer data)
{
  g_queue_push_head (hq->queues[level], data);

  if (level <= hq->lowest_non_empty_level)
    {
      hq->lowest_non_empty_level = level;
      hq->lowest_non_empty       = hq->queues[level];
    }
}

static inline gpointer
HQ_pop (HQ *hq)
{
  gint i, level;
  gpointer data = NULL;

  if (hq->lowest_non_empty != NULL)
    {
      data = g_queue_pop_tail (hq->lowest_non_empty);

      if (g_queue_is_empty (hq->lowest_non_empty))
        {
          level = hq->lowest_non_empty_level;
          hq->lowest_non_empty_level = 255;
          hq->lowest_non_empty       = NULL;

          for (i = level + 1; i < 256; i++)
            if (!g_queue_is_empty (hq->queues[i]))
              {
                hq->lowest_non_empty_level = i;
                hq->lowest_non_empty       = hq->queues[i];
                break;
              }
        }
    }

  return data;
}

static void
HQ_clean (HQ *hq)
{
  gint i;

  for (i = 0; i < 256; i++)
    {
      if (!g_queue_is_empty (hq->queues[i]))
        g_printerr ("queue %u is not empty!\n", i);
      else
        g_queue_free (hq->queues[i]);
    }
}

static void
attach (GeglOperation *self)
{
  GeglOperation *operation = GEGL_OPERATION (self);
  GParamSpec    *pspec;

  pspec = g_param_spec_object ("output",
                               "Output",
                               "Output pad for generated image buffer.",
                               GEGL_TYPE_BUFFER,
                               G_PARAM_READABLE |
                               GEGL_PARAM_PAD_OUTPUT);
  gegl_operation_create_pad (operation, pspec);
  g_param_spec_sink (pspec);

  pspec = g_param_spec_object ("input",
                               "Input",
                               "Input pad, for image buffer input.",
                               GEGL_TYPE_BUFFER,
                               G_PARAM_READWRITE |
                               GEGL_PARAM_PAD_INPUT);
  gegl_operation_create_pad (operation, pspec);
  g_param_spec_sink (pspec);

  pspec = g_param_spec_object ("aux",
                               "Aux",
                               "Auxiliary image buffer input pad.",
                               GEGL_TYPE_BUFFER,
                               G_PARAM_READWRITE |
                               GEGL_PARAM_PAD_INPUT);
  gegl_operation_create_pad (operation, pspec);
  g_param_spec_sink (pspec);
}

static void
prepare (GeglOperation *operation)
{
  const Babl  *labels_format   = babl_format ("YA u32");
  const Babl  *gradient_format = babl_format ("Y u8");

  gegl_operation_set_format (operation, "input",  labels_format);
  gegl_operation_set_format (operation, "output", labels_format);
  gegl_operation_set_format (operation, "aux",    gradient_format);
}

static GeglRectangle
get_bounding_box (GeglOperation *operation)
{
  GeglRectangle *region;

  region = gegl_operation_source_get_bounding_box (operation, "input");

  if (region != NULL)
    return *region;
  else
    return *GEGL_RECTANGLE (0, 0, 0, 0);
}

static GeglRectangle
get_required_for_output (GeglOperation       *operation,
                         const gchar         *input_pad,
                         const GeglRectangle *roi)
{
  return get_bounding_box (operation);
}

static GeglRectangle
get_invalidated_by_change (GeglOperation       *operation,
                           const gchar         *input_pad,
                           const GeglRectangle *input_region)
{
  return get_bounding_box (operation);
}

static GeglRectangle
get_cached_region (GeglOperation       *operation,
                   const GeglRectangle *roi)
{
  return get_bounding_box (operation);
}

static gboolean
process (GeglOperation       *operation,
         GeglBuffer          *input,
         GeglBuffer          *aux,
         GeglBuffer          *output,
         const GeglRectangle *result,
         gint                 level)
{
  HQ      hq;
  guint32 square3x3[18];
  gint    j;
  gint    x, y;
  GeglBufferIterator  *iter;
  GeglSampler         *gradient_sampler;
  const GeglRectangle *extent = gegl_buffer_get_extent (input);

  const Babl  *labels_format   = babl_format ("YA u32");
  const Babl  *gradient_format = babl_format ("Y u8");

  gint neighbors_coords[8][2] = {{-1, -1},{0, -1},{1, -1},
                                 {-1, 0},         {1, 0},
                                 {-1, 1}, {0, 1}, {1, 1}};

  /* initialize hierarchical queues */

  HQ_init (&hq);

  iter = gegl_buffer_iterator_new (input, extent, 0, labels_format,
                                   GEGL_ACCESS_READ, GEGL_ABYSS_NONE);

  gegl_buffer_iterator_add (iter, aux, extent, 0, gradient_format,
                            GEGL_ACCESS_READ, GEGL_ABYSS_NONE);

  gegl_buffer_iterator_add (iter, output, extent, 0, labels_format,
                            GEGL_ACCESS_WRITE, GEGL_ABYSS_NONE);

  while (gegl_buffer_iterator_next (iter))
    {
      guint32  *label    = iter->data[0];
      guint8   *pixel    = iter->data[1];
      guint32  *outlabel = iter->data[2];

      for (y = iter->roi->y; y < iter->roi->y + iter->roi->height; y++)
        for (x = iter->roi->x; x < iter->roi->x + iter->roi->width; x++)
          {
            if (label[1] != 0)
              {
                PixelCoords *p = g_new (PixelCoords, 1);
                p->x = x;
                p->y = y;

                HQ_push (&hq, *pixel, p);
              }

            outlabel[0] = label[0];
            outlabel[1] = label[1];

            pixel++;
            label += 2;
            outlabel += 2;
          }
    }

  gradient_sampler = gegl_buffer_sampler_new_at_level (aux,
                                                       gradient_format,
                                                       GEGL_SAMPLER_NEAREST,
                                                       level);
  while (!HQ_is_empty (&hq))
    {
      PixelCoords *p = (PixelCoords *) HQ_pop (&hq);
      guint32 label[2];

      GeglRectangle square_rect = {p->x - 1, p->y - 1, 3, 3};

      gegl_buffer_get (output, &square_rect, 1.0, labels_format,
                       square3x3,
                       GEGL_AUTO_ROWSTRIDE, GEGL_ABYSS_NONE);

      label[0] = square3x3[8];
      label[1] = square3x3[9];

      /* compute neighbors coordinate */
      for (j = 0; j < 8; j++)
        {
          guint32 *neighbor_label;
          gint nx = p->x + neighbors_coords[j][0];
          gint ny = p->y + neighbors_coords[j][1];

          if (nx < 0 || nx >= extent->width || ny < 0 || ny >= extent->height)
            continue;

          neighbor_label = square3x3 + ((neighbors_coords[j][0] + 1) + (neighbors_coords[j][1] + 1) * 3) * 2;

          if (neighbor_label[1] == 0)
            {
              guint8 gradient_value;
              GeglRectangle n_rect = {nx, ny, 1, 1};
              PixelCoords *n = g_new (PixelCoords, 1);
              n->x = nx;
              n->y = ny;

              gegl_sampler_get (gradient_sampler,
                                (gdouble) nx,
                                (gdouble) ny,
                                NULL, &gradient_value, GEGL_ABYSS_NONE);

              HQ_push (&hq, gradient_value, n);

              neighbor_label[0] = label[0];
              neighbor_label[1] = 1;

              gegl_buffer_set (output, &n_rect,
                               0, labels_format,
                               neighbor_label, GEGL_AUTO_ROWSTRIDE);
            }
        }

      g_free (p);
    }

  HQ_clean (&hq);
  return  TRUE;
}

static gboolean
operation_process (GeglOperation        *operation,
                   GeglOperationContext *context,
                   const gchar          *output_prop,
                   const GeglRectangle  *result,
                   gint                  level)
{
  GeglBuffer     *input = NULL;
  GeglBuffer     *aux;
  GeglBuffer     *output;
  gboolean        success;

  aux = gegl_operation_context_get_source (context, "aux");

  if (!aux)
    {
      success = FALSE;
    }
  else
    {
      input = gegl_operation_context_get_source (context, "input");
      output = gegl_operation_context_get_target (context, "output");

      success = process (operation, input, aux, output, result, level);
    }

  if (input != NULL)
    g_object_unref (input);

  if (aux != NULL)
    g_object_unref (aux);

  return success;
}

static void
gegl_op_class_init (GeglOpClass *klass)
{
  GeglOperationClass *operation_class;

  operation_class = GEGL_OPERATION_CLASS (klass);

  operation_class->attach                    = attach;
  operation_class->prepare                   = prepare;
  operation_class->process                   = operation_process;
  operation_class->get_bounding_box          = get_bounding_box;
  operation_class->get_required_for_output   = get_required_for_output;
  operation_class->get_invalidated_by_change = get_invalidated_by_change;
  operation_class->get_cached_region         = get_cached_region;
  operation_class->opencl_support            = FALSE;
  operation_class->threaded                  = FALSE;

  gegl_operation_class_set_keys (operation_class,
    "name",        "gegl:watershed-transform",
    "title",       _("Watershed Transform"),
    "categories",  "hidden",
    "description", _("Labels propagation by watershed transformation"),
    NULL);
}

#endif