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
#if GEGL_CHANT_PROPERTIES

gegl_chant_string  (window_title, "",
                    "Title to give window, if no title given inherits name of the pad providing input.")

#else

#define GEGL_CHANT_NAME        gtk_display
#define GEGL_CHANT_SELF        "gtk-display.c"
#define GEGL_CHANT_DESCRIPTION "Displays the input buffer in an GTK window ."
#define GEGL_CHANT_CATEGORIES  "output"

#define GEGL_CHANT_FILTER
#define GEGL_CHANT_CLASS_INIT

#include "gegl-chant.h"

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
  GeglChantOperation *self = GEGL_CHANT_OPERATION (g_object_get_data (G_OBJECT (widget), "op"));
  Priv               *priv = (Priv*)self->priv;

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
  GeglChantOperation *self = GEGL_CHANT_OPERATION (operation);

  if (!self->priv)
    {
      Priv *priv = g_malloc0 (sizeof (Priv));
      self->priv = (void*) priv;

      priv->window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
      priv->drawing_area = gtk_drawing_area_new ();
      gtk_container_add (GTK_CONTAINER (priv->window), priv->drawing_area);
      priv->width = -1;
      priv->height = -1;
      gtk_widget_set_size_request (priv->drawing_area, priv->width, priv->height);
      gtk_window_set_title (GTK_WINDOW (priv->window), self->window_title);

      g_signal_connect (G_OBJECT (priv->drawing_area), "expose_event",
                        G_CALLBACK (expose_event), priv);
      g_object_set_data (G_OBJECT (priv->drawing_area), "op", operation);


      gtk_widget_show_all (priv->window);

      priv->buf = NULL;
    }
  return (Priv*)(self->priv);
}

static gboolean
process (GeglOperation *operation,
         GeglNodeContext *context,
         const GeglRectangle *result)
{
  GeglChantOperation  *self = GEGL_CHANT_OPERATION (operation);
  GeglBuffer          *source;
  GeglBuffer          *input;
 Priv                 *priv = init_priv (operation);

  input = gegl_node_context_get_source (context, "input");

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
      if (self->window_title[0]!='\0')
        {
          gtk_window_set_title (GTK_WINDOW (priv->window), self->window_title);
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
  GeglChantOperation *self = GEGL_CHANT_OPERATION (object);
  Priv *priv = (Priv*)self->priv;

  if (priv)
    {
      gtk_widget_destroy (priv->window);
      if (priv->buf)
        g_free (priv->buf);
      g_free (priv);
      self->priv = NULL;
    }

  G_OBJECT_CLASS (g_type_class_peek_parent (G_OBJECT_GET_CLASS (object)))->dispose (object);
}

static void class_init (GeglOperationClass *klass)
{
  G_OBJECT_CLASS (klass)->dispose = dispose;
}

#endif
