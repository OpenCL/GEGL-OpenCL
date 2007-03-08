/* This file is an image processing operation for GEGL
 *
 * GEGL is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * GEGL is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with GEGL; if not, write to the
 * Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 *
 * Copyright 2006 Øyvind Kolås <pippin@gimp.org>
 */
#if GEGL_CHANT_PROPERTIES

gegl_chant_multiline (string, "Hello",
                      "String to display. (utf8)")
gegl_chant_string (font, "Sans",
                   "Font family. (utf8)")
gegl_chant_double (size, 1.0, 2048.0, 10.0,
                   "Approximate height of text in pixels.")
gegl_chant_color  (color, "white", "Color for the text (defaults to 'white')")
gegl_chant_int    (wrap, -1, 1000000, -1,
                   "Sets the width in pixels at which long lines will wrap. Use -1 for no wrapping.")
gegl_chant_int    (alignment, 0, 2, 0,
                   "Alignment for multi-line text (0=Left, 1=Center, 2=Right)")
gegl_chant_int    (width, 0, 1000000, 0,
                   "Rendered width in pixels. (read only)")
gegl_chant_int    (height, 0, 1000000, 0,
                   "Rendered height in pixels. (read only)")

#else

#define GEGL_CHANT_SOURCE
#define GEGL_CHANT_NAME            text
#define GEGL_CHANT_DESCRIPTION     "Display a string of text using pango and cairo."
#define GEGL_CHANT_SELF            "text.c"
#define GEGL_CHANT_CATEGORIES      "render"
#define GEGL_CHANT_CLASS_INIT
#include "gegl-chant.h"

#include <cairo.h>
#include <pango/pango-attributes.h>
#include <pango/pangocairo.h>

typedef struct {
  gchar         *string;
  gchar         *font;
  gdouble        size;
  gint           wrap;
  gint           alignment;
  GeglRectangle  defined;
} CachedExtent;

static void text_layout_text (GeglChantOperation *self,
                              cairo_t       *cr,
                              gdouble        rowstride,
                              gdouble       *width,
                              gdouble       *height)
{
  PangoFontDescription *desc;
  PangoLayout    *layout;
  PangoAttrList  *attrs;
  PangoAttribute *attr  = NULL;
  gchar          *string;
  gfloat          color[4];
  gint            alignment = 0;

  /* Create a PangoLayout, set the font and text */
  layout = pango_cairo_create_layout (cr);

  string = g_strcompress (self->string);
  pango_layout_set_text (layout, string, -1);
  g_free (string);

  desc = pango_font_description_from_string (self->font);
  pango_font_description_set_absolute_size (desc, self->size * PANGO_SCALE);
  pango_layout_set_font_description (layout, desc);

  switch (self->alignment)
  {
  case 0:
    alignment = PANGO_ALIGN_LEFT;
    break;
  case 1:
    alignment = PANGO_ALIGN_CENTER;
    break;
  case 2:
    alignment = PANGO_ALIGN_RIGHT;
    break;
  }
  pango_layout_set_alignment (layout, alignment);
  pango_layout_set_width (layout, self->wrap * PANGO_SCALE);

  attrs = pango_attr_list_new ();
  if (attrs)
  {
    gegl_color_get_rgba (self->color,
                         &color[0], &color[1], &color[2], &color[3]);
    attr = pango_attr_foreground_new ((guint16) (color[0] * 65535),
                                      (guint16) (color[1] * 65535),
                                      (guint16) (color[2] * 65535));
    if (attr)
    {
      attr->start_index = 0;
      attr->end_index   = -1;
      pango_attr_list_insert (attrs, attr);
      pango_layout_set_attributes (layout, attrs);
    }
  }

  /* Inform Pango to re-layout the text with the new transformation */
  pango_cairo_update_layout (cr, layout);

  if (width && height)
    {
      int w, h;

      pango_layout_get_pixel_size (layout, &w, &h);
      *width = (gdouble)w;
      *height = (gdouble)h;
    }
  else
    {
      /* FIXME: This feels like a hack but it stops the rendered text  */
      /* from shifting position depending on the value of 'alignment'. */
      if (self->alignment == 1)
         cairo_move_to (cr, self->width / 2, 0);
      else
        {
          if (self->alignment == 2)
             cairo_move_to (cr, self->width, 0);
        }

      pango_cairo_show_layout (cr, layout);
    }

  pango_font_description_free (desc);
  pango_attr_list_unref (attrs);
  g_object_unref (layout);
}

static gboolean
process (GeglOperation *operation,
         gpointer       context_id)
{
  GeglChantOperation *self = GEGL_CHANT_OPERATION (operation);
  GeglBuffer         *output = NULL;
  GeglRectangle      *result;

  result = gegl_operation_result_rect (operation, context_id);

  output = g_object_new (GEGL_TYPE_BUFFER,
                         "format", babl_format ("B'aG'aR'aA u8"),
                         "x",      result->x,
                         "y",      result->y,
                         "width",  result->width ,
                         "height", result->height,
                         NULL);

  {
    guchar *data = g_malloc0 (result->width * result->height * 4);
    cairo_t *cr;

    cairo_surface_t *surface = cairo_image_surface_create_for_data (data, CAIRO_FORMAT_ARGB32,
      result->width, result->height, result->width * 4);
    cr = cairo_create (surface);
    cairo_set_source_rgba (cr, 1.0, 1.0, 1.0, 1.0);
    cairo_translate (cr, -result->x, -result->y);
    text_layout_text (self, cr, 0, NULL, NULL);

    gegl_buffer_set (output, NULL, babl_format ("B'aG'aR'aA u8"), data);

    cairo_destroy (cr);
    cairo_surface_destroy (surface);
    g_free (data);
  }
  gegl_operation_set_data (operation, context_id, "output", G_OBJECT (output));

  return  TRUE;
}

static GeglRectangle
get_defined_region (GeglOperation *operation)
{
  GeglChantOperation *self = GEGL_CHANT_OPERATION (operation);
  CachedExtent *extent;
  gint status = FALSE;

  extent = (CachedExtent*)self->priv;
  if (!self->priv)
    {
      self->priv = g_malloc0 (sizeof (CachedExtent));
      extent = (CachedExtent*)self->priv;
      extent->string = g_strdup ("");
      extent->font = g_strdup ("");
    }

  if (strcmp (extent->string, self->string) ||
      strcmp (extent->font, self->font) ||
      extent->size != self->size ||
      extent->wrap != self->wrap ||
      extent->alignment != self->alignment)
    { /* get extents */
      cairo_t *cr;
      gdouble width, height;

      cairo_surface_t *surface  = cairo_image_surface_create (CAIRO_FORMAT_ARGB32,
          1, 1);
      cr = cairo_create (surface);
      text_layout_text (self, cr, 0, &width, &height);
      cairo_destroy (cr);
      cairo_surface_destroy (surface);

      extent->defined.width = width;
      extent->defined.height = height;
      g_free (extent->string);
      extent->string = g_strdup (self->string);
      g_free (extent->font);
      extent->font = g_strdup (self->font);
      extent->size = self->size;
      extent->wrap = self->wrap;
      extent->alignment = self->alignment;

      /* store the measured size for later use */
      self->width = width;
      self->height = height;
    }

  if (status)
    {
      g_warning ("get defined region for text '%s' failed", self->string);
    }

  return extent->defined;
}

static void
finalize (GObject *object)
{
  GeglChantOperation *self = GEGL_CHANT_OPERATION (object);
    
  if (self->priv)
    {
      CachedExtent *extent = (CachedExtent*)self->priv;
      g_free (extent->string);
      g_free (extent->font);
      g_free (extent);
    }

  G_OBJECT_CLASS (g_type_class_peek_parent (G_OBJECT_GET_CLASS (object)))->finalize (object);
}

static void class_init (GeglOperationClass *klass)
{
  G_OBJECT_CLASS (klass)->finalize = finalize;
}

#endif
