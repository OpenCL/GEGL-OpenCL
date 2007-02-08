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
#include "gegl-chant.h"

#include <cairo.h>
#include <pango/pango-attributes.h>
#include <pango/pangocairo.h>

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
                         "format", /* FIXME: when babl requirements is
                                      bumped to 0.0.14 the name should
                                      be enough */
                         babl_format_new ("name", "B'aG'aR'aA u8",
                                          babl_model ("R'aG'aB'aA"),
                                          babl_type ("u8"),
                                          babl_component ("B'a"),
                                          babl_component ("G'a"),
                                          babl_component ("R'a"),
                                          babl_component ("A"),
                                          NULL),
                         "x",      result->x,
                         "y",      result->y,
                         "width",  result->w,
                         "height", result->h,
                         NULL);

  {
    guchar *data = g_malloc0 (result->w * result->h * 4);
    cairo_t *cr;

    cairo_surface_t *surface = cairo_image_surface_create_for_data (data, CAIRO_FORMAT_ARGB32,
      result->w, result->h, result->w* 4);
    cr = cairo_create (surface);
    cairo_set_source_rgba (cr, 1.0, 1.0, 1.0, 1.0);
    cairo_translate (cr, -result->x, -result->y);
    text_layout_text (self, cr, 0, NULL, NULL);

    gegl_buffer_set (output,
                     NULL,
                     babl_format_new ("name", "B'aG'aR'aA u8",
                                      babl_model ("R'aG'aB'aA"),
                                      babl_type ("u8"),
                                      babl_component ("B'a"),
                                      babl_component ("G'a"),
                                      babl_component ("R'a"),
                                      babl_component ("A"),
                                      NULL),
                     data);

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
  GeglRectangle result = {0,0,0,0};
  GeglChantOperation *self = GEGL_CHANT_OPERATION (operation);
  gint status = FALSE;

  { /* get extents */
    cairo_t *cr;
    gdouble width, height;

    cairo_surface_t *surface  = cairo_image_surface_create (CAIRO_FORMAT_ARGB32,
        1, 1);
    cr = cairo_create (surface);
    text_layout_text (self, cr, 0, &width, &height);
    result.w = width;
    result.h = height;

    /* store the measured size for later use */
    self->width = width;
    self->height = height;

    cairo_destroy (cr);
    cairo_surface_destroy (surface);
  }

  if (status)
    {
      g_warning ("get defined region for text '%s' failed", self->string);
    }

  return result;
}

#endif
