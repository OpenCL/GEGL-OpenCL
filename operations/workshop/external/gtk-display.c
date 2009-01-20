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
 */

#include "config.h"
#include <glib/gi18n-lib.h>


#ifdef GEGL_CHANT_PROPERTIES

gegl_chant_string  (window_title, _("Window Title"), "",
                    _("Title to give window, if no title given inherits name of the pad providing input."))

#else

#define GEGL_CHANT_TYPE_FILTER
#define GEGL_CHANT_C_FILE       "gtk-display.c"

#include "gegl-chant.h"
#include "graph/gegl-node.h"

#include <gtk/gtk.h>

typedef struct
{
  GtkWidget *window;
  GtkWidget *drawing_area;
  gint       width;
  gint       height;
  guchar    *buf;
} Priv;

static gboolean
expose_event (GtkWidget *widget, GdkEventExpose * event)
{
  GeglChantO *o = GEGL_CHANT_PROPERTIES (g_object_get_data (G_OBJECT (widget), "op"));
  Priv       *priv = (Priv*)o->chant_data;

  if (priv->buf)
    {
      if (event->area.x + event->area.width > priv->width)
        event->area.width = priv->width - event->area.x;
      if (event->area.y + event->area.height > priv->height)
        event->area.height = priv->height - event->area.y;

      gdk_draw_rgb_32_image (widget->window,
                             widget->style->black_gc,
                             event->area.x, event->area.y,
                             event->area.width, event->area.height,
                             GDK_RGB_DITHER_NONE,
        priv->buf + (event->area.y * priv->width + event->area.x) * 4,
        (unsigned)(priv->width * 4));
    }
  return TRUE;
}

static Priv *init_priv (GeglOperation *operation)
{
  GeglChantO *o = GEGL_CHANT_PROPERTIES (operation);

  if (!o->chant_data)
    {
      Priv *priv = g_new0 (Priv, 1);
      o->chant_data = (void*) priv;

      priv->window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
      priv->drawing_area = gtk_drawing_area_new ();
      gtk_container_add (GTK_CONTAINER (priv->window), priv->drawing_area);
      priv->width = -1;
      priv->height = -1;
      gtk_widget_set_size_request (priv->drawing_area, priv->width, priv->height);
      gtk_window_set_title (GTK_WINDOW (priv->window), o->window_title);

      g_signal_connect (G_OBJECT (priv->drawing_area), "expose_event",
                        G_CALLBACK (expose_event), priv);
      g_object_set_data (G_OBJECT (priv->drawing_area), "op", operation);


      gtk_widget_show_all (priv->window);

      priv->buf = NULL;
    }
  return (Priv*)(o->chant_data);
}

static gboolean
process (GeglOperation       *operation,
         GeglBuffer          *input,
         GeglBuffer          *source,
         const GeglRectangle *result)
{
  GeglChantO *o = GEGL_CHANT_PROPERTIES (operation);
  Priv       *priv = init_priv (operation);

  g_assert (input);

  if (priv->width != result->width  ||
      priv->height != result->height)
    {
      priv->width = result->width ;
      priv->height = result->height;
      gtk_widget_set_size_request (priv->drawing_area, priv->width, priv->height);

      if (priv->buf)
        g_free (priv->buf);
      priv->buf = g_malloc (priv->width * priv->height * 4);
    }

  source = gegl_buffer_create_sub_buffer (input, result);

  gegl_buffer_get (source, 1.0, NULL, babl_format ("R'G'B'A u8"), priv->buf, GEGL_AUTO_ROWSTRIDE);
  gtk_widget_queue_draw (priv->drawing_area);

  if (priv->window)
    {
      gtk_window_resize (GTK_WINDOW (priv->window), priv->width, priv->height);
      if (o->window_title[0]!='\0')
        {
          gtk_window_set_title (GTK_WINDOW (priv->window), o->window_title);
        }
      else
        {
          gtk_window_set_title (GTK_WINDOW (priv->window),
           gegl_node_get_debug_name (gegl_node_get_producer(operation->node, "input", NULL))
           );
        }

      if (0)while (gtk_events_pending ())
        {
          gtk_main_iteration ();
        }
    }
  g_object_unref (source);

  return  TRUE;
}

static void
dispose (GObject *object)
{
  GeglChantO *o = GEGL_CHANT_PROPERTIES (object);
  Priv       *priv = (Priv*)o->chant_data;

  if (priv)
    {
      gtk_widget_destroy (priv->window);
      if (priv->buf)
        g_free (priv->buf);
      g_free (priv);
      o->chant_data = NULL;
    }

  G_OBJECT_CLASS (g_type_class_peek_parent (G_OBJECT_GET_CLASS (object)))->dispose (object);
}


static void
gegl_chant_class_init (GeglChantClass *klass)
{
  GeglOperationClass       *operation_class;
  GeglOperationFilterClass *filter_class;

  operation_class = GEGL_OPERATION_CLASS (klass);
  filter_class    = GEGL_OPERATION_FILTER_CLASS (klass);

  filter_class->process = process;
  G_OBJECT_CLASS (klass)->dispose = dispose;

  operation_class->name        = "gegl:gtk-display";
  operation_class->categories  = "output";
  operation_class->description =
        _("Displays the input buffer in an GTK window .");
}

#endif
