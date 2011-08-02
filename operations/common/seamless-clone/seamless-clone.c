/* This file is an image processing operation for GEGL
 *
 * seamless-clone.c
 * Copyright (C) 2011 Barak Itkin <lightningismyname@gmail.com>
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
 */

#ifdef GEGL_CHANT_PROPERTIES
#else

#define GEGL_CHANT_TYPE_COMPOSER
#define GEGL_CHANT_C_FILE       "seamless-clone.c"

#include "config.h"
#include <glib/gi18n-lib.h>
#include "gegl-chant.h"

#include <stdio.h> /* TODO: get rid of this debugging way! */

#include "poly2tri-c/poly2tri.h"
#include "poly2tri-c/refine/triangulation.h"
#include "poly2tri-c/render/mesh-render.h"
#include "seamless-clone.h"

static GeglRectangle
get_required_for_output (GeglOperation       *operation,
                         const gchar         *input_pad,
                         const GeglRectangle *region)
{
  GeglRectangle result;

  if (g_strcmp0 (input_pad, "input"))
    result = *gegl_operation_source_get_bounding_box (operation, "input");
  else if (g_strcmp0 (input_pad, "aux"))
    result = *gegl_operation_source_get_bounding_box (operation, "aux");
  else
    g_assert_not_reached ();

  printf ("Input \"%s\" size is:\n", input_pad);
  gegl_rectangle_dump (&result);

  return result;
}

static void
prepare (GeglOperation *operation)
{
  Babl *format = babl_format ("RGBA float");

  gegl_operation_set_format (operation, "input",  format);
  gegl_operation_set_format (operation, "aux",    format);
  gegl_operation_set_format (operation, "output", format);
}


typedef struct {
  GeglBuffer     *aux_buf;
  GeglBuffer     *input_buf;
  ScMeshSampling *sampling;
} ScColorComputeInfo;

static void
sc_point_to_color_func (P2tRPoint *point,
                        gfloat    *dest,
                        gpointer   cci_p)
{
  ScColorComputeInfo *cci = (ScColorComputeInfo*) cci_p;
  ScSampleList       *sl = g_hash_table_lookup (cci->sampling, point);
  gfloat aux_c[4], input_c[4], dest_c[3] = {0, 0, 0};
  gint i;
  guint N = sl->points->len;
  
  Babl *format = babl_format("RGB float");

  for (i = 0; i < N; i++)
    {
      P2tRPoint *pt = g_ptr_array_index (sl->points, i);
      gdouble weight = g_array_index (sl->weights, gdouble, i);
      
      gegl_buffer_sample (cci->aux_buf, pt->x, pt->y, NULL, aux_c, format, GEGL_INTERPOLATION_NEAREST);
      gegl_buffer_sample (cci->input_buf, pt->x, pt->y, NULL, input_c, format, GEGL_INTERPOLATION_NEAREST);
      
      dest_c[0] = weight * (input_c[0] - aux_c[0]);
      dest_c[1] = weight * (input_c[1] - aux_c[1]);
      dest_c[2] = weight * (input_c[2] - aux_c[2]);
	}

  dest[0] = dest_c[0] / sl->total_weight;
  dest[1] = dest_c[1] / sl->total_weight;
  dest[2] = dest_c[2] / sl->total_weight;
  dest[3] = 1;
}

static gboolean
process (GeglOperation       *operation,
         GeglBuffer          *input,
         GeglBuffer          *aux,
         GeglBuffer          *output,
         const GeglRectangle *result)
{
  gfloat    *aux_raw, *out_raw;

  GeglRectangle aux_rect = *gegl_operation_source_get_bounding_box (operation, "aux");

  ScOutline          *outline;
  
  P2tRTriangulation  *mesh;
  GeglRectangle       mesh_bounds;

  ScMeshSampling     *mesh_sampling;

  ScColorComputeInfo  cci;
  P2tRImageConfig     imcfg;

  printf ("The aux_rect is:\n");
  gegl_rectangle_dump (&aux_rect);

  /********************************************************************/
  /* Part 1: The preprocessing                                        */
  /********************************************************************/
  
  /* First, find the paste outline */
  aux_raw = g_new (gfloat, 4 * aux_rect.width * aux_rect.height);
  gegl_buffer_get (aux, 1.0, &aux_rect, babl_format("RGBA float"), aux_raw, GEGL_AUTO_ROWSTRIDE);
  
  outline = sc_outline_find_ccw (&aux_rect, aux_raw);
  
  g_free (aux_raw);
  aux_raw = NULL;

  /* Then, Generate the mesh */
  mesh = sc_make_fine_mesh (outline, &mesh_bounds);

  /* Finally, Generate the mesh sample list for each point */
  mesh_sampling = sc_mesh_sampling_compute (outline, mesh);

  /* If caching of UV is desired, it shold be done here! */

  /********************************************************************/
  /* Part 2: The rendering                                            */
  /********************************************************************/

  /* Alocate the output buffer */
  out_raw = g_new (gfloat, 4 * result->width * result->height);

  /* Render the mesh into it */
  cci.aux_buf = aux;
  cci.input_buf = input;
  cci.sampling = mesh_sampling;

  imcfg.min_x = result->x;
  imcfg.min_y = result->y;
  imcfg.step_x = imcfg.step_y = 1;
  imcfg.x_samples = result->width;
  imcfg.y_samples = result->height;
  imcfg.cpp = 4;
  
  p2tr_mesh_render_scanline (mesh, out_raw, &imcfg, sc_point_to_color_func, &cci);

  /* TODO: Add the aux to the mesh rendering! */
  gegl_buffer_set (output, result, babl_format("RGBA float"), out_raw, GEGL_AUTO_ROWSTRIDE);

  /* Free memory, by the order things were allocated! */
  g_free (out_raw);
  sc_mesh_sampling_free (mesh_sampling);
  p2tr_triangulation_free (mesh);
  sc_outline_free (outline);
  
  return  TRUE;
}

static void
gegl_chant_class_init (GeglChantClass *klass)
{
  GeglOperationClass         *operation_class = GEGL_OPERATION_CLASS (klass);
  GeglOperationComposerClass *composer_class  = GEGL_OPERATION_COMPOSER_CLASS (klass);

  operation_class->prepare     = prepare;
  operation_class->name        = "gegl:seamless-clone";
  operation_class->categories  = "blend";
  operation_class->description = "Seamless cloning operation";
  operation_class->get_required_for_output = get_required_for_output;

  composer_class->process      = process;
}


#endif
