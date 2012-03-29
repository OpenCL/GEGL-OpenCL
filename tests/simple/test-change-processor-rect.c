/*
 * This program is free software; you can redistribute it and/or modify
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
 * along with this program; if not, see <http://www.gnu.org/licenses/>.
 *
 * Copyright (C) 2009 Martin Nordholts
 */

#include "config.h"
#include <string.h>
#include <math.h>

#include "gegl.h"

#define SUCCESS  0
#define FAILURE -1

#define FLOATS_EQUAL(x,y) (fabs((x) - (y)) < 0.00001f)


static gboolean
test_change_processor_rect_do_test (GeglProcessor       *processor,
                                    const GeglRectangle *rect,
                                    GeglNode            *sink)
{
  gint        i                         = 0;
  gboolean    result                    = TRUE;
  float       expected_result_buffer[4] = { 1.0, 1.0, 1.0, 1.0 };
  float       result_buffer[4]          = { 0, };
  GeglBuffer *buffer                    = NULL;

  gegl_node_set (sink,
                 "buffer", &buffer,
                 NULL);
  
  gegl_processor_set_rectangle (processor, rect);

  while (gegl_processor_work (processor, NULL));

  gegl_buffer_get (buffer,
                   rect,
                   1.0,
                   babl_format ("RGBA float"),
                   result_buffer,
                   GEGL_AUTO_ROWSTRIDE,
                   GEGL_ABYSS_NONE);

  /* Compare with a small epsilon to account for accumulated error */
  for(i = 0; i < G_N_ELEMENTS (expected_result_buffer); i++)
    result = result && FLOATS_EQUAL (expected_result_buffer[i], result_buffer[i]);

  gegl_node_set (sink,
                 "buffer", NULL,
                 NULL);

  g_object_unref (buffer);

  return result;
}

int main(int argc, char *argv[])
{
  gint           result       = SUCCESS;
  GeglRectangle  rect1        = { 0, 0, 1, 1 };
  GeglRectangle  rect2        = { 1, 0, 1, 1 };
  GeglRectangle  rect3        = { 1, 1, 1, 1 };
  GeglRectangle  rect4        = { 0, 1, 1, 1 };
  GeglColor     *common_color = NULL;
  GeglNode      *gegl         = NULL;
  GeglNode      *color        = NULL;
  GeglNode      *layer        = NULL;
  GeglNode      *text         = NULL;
  GeglNode      *sink         = NULL;
  GeglProcessor *processor    = NULL;

  /* Convenient line to keep around:
  g_log_set_always_fatal (G_LOG_LEVEL_WARNING | G_LOG_LEVEL_CRITICAL);
   */

  g_thread_init (NULL);
  gegl_init (&argc, &argv);

  common_color = gegl_color_new ("rgb(1.0, 1.0, 1.0)");

  gegl         = gegl_node_new ();
  color        = gegl_node_new_child (gegl,
                                      "operation", "gegl:color",
                                      "value", common_color,
                                      NULL);
  layer        = gegl_node_new_child (gegl,
                                      "operation", "gegl:layer",
                                      "x", 0.0,
                                      "y", 0.0,
                                      NULL);
  text         = gegl_node_new_child (gegl,
                                      "operation", "gegl:text",
                                      "color", common_color,
                                      "string", "█████████████████████████",
                                      "size", 200.0,
                                      NULL);
  sink         = gegl_node_new_child (gegl,
                                      "operation", "gegl:buffer-sink",
                                      NULL);

  /* We build our graph for processing complexity, not for compositing
   * complexity
   */
  gegl_node_link_many (color, layer, sink, NULL);
  gegl_node_connect_to (text, "output",  layer, "aux");

  /* Create a processor */
  processor = gegl_node_new_processor (sink, NULL);

  /* Do the tests */
  if (!test_change_processor_rect_do_test (processor, &rect1, sink))
    {
      g_printerr ("test-change-processor-rect: First compare failed\n");
      result = FAILURE;
      goto abort;
    }
  if (!test_change_processor_rect_do_test (processor, &rect2, sink))
    {
      g_printerr ("test-change-processor-rect: Second compare failed\n");
      result = FAILURE;
      goto abort;
    }
  if (!test_change_processor_rect_do_test (processor, &rect3, sink))
    {
      g_printerr ("test-change-processor-rect: Third compare failed\n");
      result = FAILURE;
      goto abort;
    }
  if (!test_change_processor_rect_do_test (processor, &rect4, sink))
    {
      g_printerr ("test-change-processor-rect: Fourth compare failed\n");
      result = FAILURE;
      goto abort;
    }

  /* Cleanup */
 abort:
  g_object_unref (processor);
  g_object_unref (common_color);
  g_object_unref (gegl);
  gegl_exit ();

  return result;
}
