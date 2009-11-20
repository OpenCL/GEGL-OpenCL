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
 * Copyright (C) 2003, 2004, 2006, 2008 Øyvind Kolås
 */

#include <string.h>
#include "config.h"
#include <glib.h>
#include <gegl.h>
#include <gtk/gtk.h>
#include "util/gegl-view.h"
#include "util/gegl-view.c"
#include "property-types/gegl-path.h"

#define HARDNESS     0.2
#define LINEWIDTH   60.0
#define COLOR       "rgba(0.0,0.0,0.0,0.4)"

GtkWidget         *window;
GtkWidget         *view;
static GeglBuffer *buffer   = NULL;
static GeglNode   *gegl     = NULL;
static GeglNode   *out      = NULL;
static GeglNode   *top      = NULL;
static gboolean    pen_down = FALSE;
static GeglPath   *vector   = NULL;

static GeglNode   *over     = NULL;
static GeglNode   *stroke   = NULL;


static gboolean paint_press (GtkWidget      *widget,
                             GdkEventButton *event)
{
  if (event->button == 1)
    {
      vector     = gegl_path_new ();

      over       = gegl_node_new_child (gegl, "operation", "gegl:over", NULL);
      stroke     = gegl_node_new_child (gegl, "operation", "gegl:path",
                                        "d", vector,
                                        "fill-opacity", 0.0,
                                        "stroke", gegl_color_new (COLOR),
                                        "stroke-width", LINEWIDTH,
                                        "stroke-hardness", HARDNESS,
                                        NULL);
      gegl_node_link_many (top, over, out, NULL);
      gegl_node_connect_to (stroke, "output", over, "aux");
      gegl_path_append (vector, 'M', event->x, event->y);

      pen_down = TRUE;

      return TRUE;
    }
  return FALSE;
}


static gboolean paint_motion (GtkWidget      *widget,
                              GdkEventMotion *event)
{
  if (event->state & GDK_BUTTON1_MASK)
    {
      if (!pen_down)
        {
          return TRUE;
        }

      gegl_path_append (vector, 'L', event->x, event->y);
      return TRUE;
    }
  return FALSE;
}


static gboolean paint_release (GtkWidget      *widget,
                               GdkEventButton *event)
{
  if (event->button == 1)
    {
      gdouble        x0, x1, y0, y1;
      GeglProcessor *processor;
      GeglNode      *writebuf;
      GeglRectangle  roi;

      gegl_path_get_bounds (vector, &x0, &x1, &y0, &y1);

      roi.x = x0 - LINEWIDTH;
      roi.y = y0 - LINEWIDTH;
      roi.width = x1 - x0 + LINEWIDTH * 2;
      roi.height = y1 - y0 + LINEWIDTH * 2;

      writebuf = gegl_node_new_child (gegl,
                                      "operation", "gegl:write-buffer",
                                      "buffer",    buffer,
                                      NULL);
      gegl_node_link_many (over, writebuf, NULL);

      processor = gegl_node_new_processor (writebuf, &roi);
      while (gegl_processor_work (processor, NULL)) ;

      gegl_processor_destroy (processor);
      g_object_unref (writebuf);

      gegl_node_link_many (top, out, NULL);
      g_object_unref (over);
      g_object_unref (stroke);

      over     = NULL;
      stroke   = NULL;
      pen_down = FALSE;

      return TRUE;
    }
  return FALSE;
}

gint
main (gint    argc,
      gchar **argv)
{

  g_thread_init (NULL);
  gtk_init (&argc, &argv);
  gegl_init (&argc, &argv);

  window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  gtk_window_set_title (GTK_WINDOW (window), "GEGL destructive painter");

  if (argv[1] == NULL)
    {
      GeglRectangle rect = {0, 0, 512, 512};
      gpointer buf;

      /* XXX: the format should be RaGaBaA float, it is nonpremultiplied
       * right now, slowing things down a bit, but it circumvents overeager
       * in place processing code.
       */
      buffer = gegl_buffer_new (&rect, babl_format("RGBA float"));
      buf    = gegl_buffer_linear_open (buffer, NULL, NULL, babl_format ("Y' u8"));
      /* it would be useful to have a programmatic way of doing this */
      memset (buf, 255, 512 * 512);
      gegl_buffer_linear_close (buffer, buf);
    }
  else
    {
      buffer = gegl_buffer_open (argv[1]);
    }

  gegl = gegl_node_new ();
  {
    GeglNode *loadbuf = gegl_node_new_child (gegl, "operation", "gegl:buffer-source", "buffer", buffer, NULL);
    out  = gegl_node_new_child (gegl, "operation", "gegl:nop", NULL);

    gegl_node_link_many (loadbuf, out, NULL);

    view = g_object_new (GEGL_TYPE_VIEW, "node", out, NULL);
    top  = loadbuf;
  }

  g_signal_connect (GTK_OBJECT (view), "motion-notify-event",
                    (GCallback) paint_motion, NULL);
  g_signal_connect (GTK_OBJECT (view), "button-press-event",
                    (GCallback) paint_press, NULL);
  g_signal_connect (GTK_OBJECT (view), "button-release-event",
                    (GCallback) paint_release, NULL);
  gtk_widget_add_events (view, GDK_BUTTON_RELEASE_MASK);

  gtk_container_add (GTK_CONTAINER (window), view);
  gtk_widget_set_size_request (view, 512, 512);

  g_signal_connect (G_OBJECT (window), "delete-event",
                    G_CALLBACK (gtk_main_quit), window);
  gtk_widget_show_all (window);

  gtk_main ();
  g_object_unref (gegl);
  gegl_buffer_destroy (buffer);

  gegl_exit ();
  return 0;
}
