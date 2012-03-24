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
gegl_chant_int (max_refine_steps, _("Refinement Steps"), 0, 100000.0, 2000,
                _("Maximal amount of refinement points to be used for the interpolation mesh"))

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
  GeglRectangle *temp = NULL;
  GeglRectangle  result;

  g_debug ("seamless-clone.c::get_required_for_output");
  
  if (g_strcmp0 (input_pad, "input") || g_strcmp0 (input_pad, "aux"))
    temp = gegl_operation_source_get_bounding_box (operation, input_pad);
  else
    g_warning ("seamless-clone::Unknown input pad \"%s\"\n", input_pad);

  if (temp != NULL)
    result = *temp;
  else
    {
      result.width = result.height = 0;
    }
  
  return result;
}

static void
prepare (GeglOperation *operation)
{
  Babl *format = babl_format ("R'G'B'A float");

  g_debug ("seamless-clone.c::prepare");
  
  gegl_operation_set_format (operation, "input",  format);
  gegl_operation_set_format (operation, "aux",    format);
  gegl_operation_set_format (operation, "output", format);
}


typedef struct {
  GeglBuffer     *aux_buf;
  GeglBuffer     *input_buf;
  ScMeshSampling *sampling;
  GHashTable     *pt2col;
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
  gdouble weightT = 0;
  guint N = sl->points->len;
  gfloat *col_cpy;

  Babl *format = babl_format ("R'G'B'A float");

  if ((col_cpy = g_hash_table_lookup (cci->pt2col, point)) != NULL)
    {
      dest[0] = col_cpy[0];
      dest[1] = col_cpy[1];
      dest[2] = col_cpy[2];
      dest[3] = col_cpy[3];
      return;
    }
  else
    {
      col_cpy = g_new (gfloat, 4);
      g_hash_table_insert (cci->pt2col, point, col_cpy);
    }

  for (i = 0; i < N; i++)
    {
      ScPoint *pt = g_ptr_array_index (sl->points, i);
      gdouble weight = g_array_index (sl->weights, gdouble, i);
      // g_print ("%f+",weight);

      gegl_buffer_sample (cci->aux_buf, pt->x, pt->y, NULL, aux_c, format, GEGL_SAMPLER_NEAREST);
      gegl_buffer_sample (cci->input_buf, pt->x, pt->y, NULL, input_c, format, GEGL_SAMPLER_NEAREST);
      
      dest_c[0] += weight * (input_c[0] - aux_c[0]);
      dest_c[1] += weight * (input_c[1] - aux_c[1]);
      dest_c[2] += weight * (input_c[2] - aux_c[2]);
      weightT += weight;
	}

  // g_print ("=%f\n",weightT);
  col_cpy[0] = dest[0] = dest_c[0] / weightT;
  col_cpy[1] = dest[1] = dest_c[1] / weightT;
  col_cpy[2] = dest[2] = dest_c[2] / weightT;
  col_cpy[3] = dest[3] = 1;
  //g_print ("(%f,%f,%f)",dest[0],dest[1],dest[2]);
}

static gboolean
process (GeglOperation       *operation,
         GeglBuffer          *input,
         GeglBuffer          *aux,
         GeglBuffer          *output,
         const GeglRectangle *result)
{
  gfloat    *out_raw, *pixel;
  gdouble    x, y;

  GeglRectangle aux_rect = *gegl_operation_source_get_bounding_box (operation, "aux");
  GeglRectangle       to_render;

  ScOutline          *outline;
  
  P2tRTriangulation  *mesh;
  GeglRectangle       mesh_bounds;

  ScMeshSampling     *mesh_sampling;

  ScColorComputeInfo  cci;
  P2tRImageConfig     imcfg;

  Babl               *format = babl_format("R'G'B'A float");
  int                 max_refine_steps = GEGL_CHANT_PROPERTIES (operation)->max_refine_steps;

  g_debug ("seamless-clone.c::process");
  printf ("The aux_rect is: ");
  gegl_rectangle_dump (&aux_rect);

  /********************************************************************/
  /* Part 1: The preprocessing                                        */
  /********************************************************************/

  /* First, find the paste outline */
  g_debug ("Start making outline");
  outline = sc_outline_find_ccw (&aux_rect, aux);
  g_debug ("Finish making outline");

  /* Then, Generate the mesh */
  g_debug ("Start making fine mesh with at most %d points", max_refine_steps);
  mesh = sc_make_fine_mesh (outline, &mesh_bounds, max_refine_steps);
  g_debug ("Finish making fine mesh");

  /* Finally, Generate the mesh sample list for each point */
  g_debug ("Start computing sampling");
  mesh_sampling = sc_mesh_sampling_compute (outline, mesh);
  g_debug ("Finish computing sampling");

  /* If caching of UV is desired, it shold be done here! */

  /********************************************************************/
  /* Part 2: The rendering                                            */
  /********************************************************************/

  /* We only need to render the intersection of the mesh bounds and the
   * desired output */
   gegl_rectangle_intersect (&to_render, result, &mesh_bounds);

  /* Alocate the output buffer */
  out_raw = g_new (gfloat, 4 * to_render.width * to_render.height);

  /* Render the mesh into it */
  cci.aux_buf = aux;
  cci.input_buf = input;
  cci.sampling = mesh_sampling;
  cci.pt2col = g_hash_table_new_full (g_direct_hash, g_direct_equal, NULL, g_free);

  imcfg.min_x = to_render.x;
  imcfg.min_y = to_render.y;
  imcfg.step_x = imcfg.step_y = 1;
  imcfg.x_samples = to_render.width;
  imcfg.y_samples = to_render.height;
  imcfg.cpp = 4;

  g_debug ("Start mesh rendering");
  p2tr_mesh_render_scanline (mesh, out_raw, &imcfg, sc_point_to_color_func, &cci);
  g_debug ("Finish mesh rendering");

  g_debug ("Start aux adding");
  pixel = out_raw;

  pixel = out_raw;
  for (y = 0; y < imcfg.y_samples; y++)
    for (x = 0; x < imcfg.x_samples; x++)
      {
        gfloat aux_c[4];
        gdouble Px = imcfg.min_x + x * imcfg.step_x;
        gdouble Py = imcfg.min_y + y * imcfg.step_y;
        gegl_buffer_sample (aux, Px, Py, NULL, aux_c, format, GEGL_SAMPLER_NEAREST);
        *pixel++ += aux_c[0];
        *pixel++ += aux_c[1];
        *pixel++ += aux_c[2];
        *pixel++;// += 0;//aux_c[3];
      }

  g_debug ("Finish aux adding");
  
  /* TODO: Add the aux to the mesh rendering! */
  gegl_buffer_set (output, &to_render, babl_format("R'G'B'A float"), out_raw, GEGL_AUTO_ROWSTRIDE);

  /* Free memory, by the order things were allocated! */
  g_hash_table_destroy (cci.pt2col);
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
